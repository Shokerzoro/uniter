

##### **0. Project setup**
This is a CMake project in Qt C++ (17 standard), in QtCreator with git in the root. It has 3 subprojects: the Common static library, the .exe for Updater updates, and the Uniter application itself. It is assembled in 2 versions with dynamic linking (for debug and profile) and static linking (RelWithDebInfo and Release). Uses GoogleTests for unit testing, which are compiled into a separate .exe and run with each build in a separate console. Added --verbose flag to display compiler calls.

Third-party libraries are used - MuPDF for extracting data from pdf files, tinyxml2 for working with XML. Moreover, they are assembled in the MSYS environment using MinGW, like the entire Uniter project on Qt, which will allow cross-platform assembly.

On the server side, Kafka is used as a message broker, storing these messages for a certain time, after which the messages are deleted and the client is forced to perform full synchronization.

MinIo is used as a storage for heavy files, while the server operates with URL links to files (mainly design files, but also pdf standards, etc.), and the client requests a URL and a token to download them on demand.


##### **0.1. Git Submodules**

The project uses git submodules to connect repositories common to the server part: a contract (protocol + resources) and a database access layer.

**List of submodules:**

| Submodule | Repository path | Repository |
|---|---|---|
| `uniter-contract` | `src/uniter/contract/` | https://github.com/Shokerzoro/uniter-contract |
| `uniter-database` | `src/uniter/database/` | https://github.com/Shokerzoro/uniter-database |

**Initial repository cloning:**

```bash
git clone --recurse-submodules https://github.com/Shokerzoro/uniter.git
```

**If the repository is already cloned without submodules:**

```bash
git submodule update --init --recursive
```

**Updating submodules to latest commit:**

```bash
git submodule update --remote --merge
```

**CMake integration:**

`uniter-contract` and `uniter-database` are included via `include()` in `src/uniter/CMakeLists.txt`:

```cmake
include(contract/sources.cmake) # defines uniter::contract
include(database/sources.cmake) # defines uniter::database
```

Each submodule provides `sources.cmake` instead of `CMakeLists.txt`, allowing them to be included in any subproject via a simple `include()` without spawning nested `cmake_minimum_required()`/`project()`.

##### **1. Application Description**

The purpose of the application is to automate the accounting of materials for manufacturing/design companies in the field of mechanical engineering. The application's operating algorithms take advantage of the ESKD, using ready-made design data to automatically extract information about products, which includes the structure of the product, designs, and materials used. At the same time, automation covers the following processes: release of design documentation, procurement, production (inventory + task management).
The design of an information system is based on the assumption that all capabilities can be represented as CRUD operations on data, including new subsystems. This allows you to scale the application and add support for other business processes.

Future plans include expanding the production unit, the ability to create workshops and route production through them, as well as read technological documentation. It is also a platform for introducing automated drafting using AI in the future.
It is also a platform for introducing cooperation within the engineering industry for publishing orders for metalworking.
##### **2. Application architectural layers**
The application is organized into four independent layers, each of which is responsible for its own area of ​​responsibility.

###### 1. **Network Layer**
The network layer ensures that all external communications are isolated from the rest of the application. It consists of three independent components, each of which is responsible for its own communication channel. All components interact with the upper layers exclusively through AppManager (application management), without direct connections with business logic or UI

###### **ServerConnector**
Channel of communication with the self-written server (authorization engine and CRUD operations). Uses TCP/SSL connection for secure data transfer.

Responsibility:
- Establishing and maintaining a permanent TCP/SSL connection with the self-written server
- Serialize UniterMessage in XML and send to server
- Deserializing XML responses back to UniterMessage, spitting them out to the top
- Buffering of outgoing CUD requests when there is no connection (for subsequent sending upon recovery)
- Handling request timeouts (each request has a time limit for waiting for a response)
Operations:
- Protocol operations (MessageType = PROTOCOL) for authentication, full synchronization request, request for access to Kafka, request for MinIO tokens
- CRUD operations (MessageType = CRUD) - requests from the client for operations on resources in all subsystems

###### **KafkaConnector**
One-way channel for receiving broadcast notifications from the Apache Kafka distributed messaging system. The client acts only as a consumer (reader) and never publishes messages.

Responsibility:

- Connect to the Kafka cluster using credentials received from the server via ServerConnector
- Subscription to the company topic, continuous reading of new messages (only CRUD broadcast notifications), deserialization in UniterMessage and “spitting” to the top
- Remembering the last processed offset in OS Secure Storage with reference to a specific User (to be able to continue reading after a restart)
- Checking the availability of the saved offset during initialization, sending an error to the top to indicate the need for full synchronization (FULL_SYNC)

###### **MinioConnector**
HTTP client for interacting with the MinIO S3 API object storage. Provides operations for uploading and downloading files (PDF drawings, XML snapshots of design documentation).
Does not have direct credentials to access MinIO. Instead, a mechanism is used for pre-signed URLs: the client requests from the server (via ServerConnector) a temporary URL with an embedded access token, valid for a short time

Responsibility:
- Performing HTTP GET/PUT/DELETE requests to MinIO via pre-signed URLs to download/upload files. Knows only atamar tasks (which file to download/upload and where)


###### 2. **Application Management Layer**
**Components**: AppManager, ConfigManager
**Responsibility**:
**AppManager**:
- Global application FSM, lifecycle management
- Processing protocol UniterMessage (for example during initialization)
- UniterMessage routing between network layer and data layer
- Coordination of FULL_SYNC execution

**ConfigManager**:
- Parsing User from the server
- Generating signals to create subsystem tab widgets
- Management of configuration changes


###### 3. **Data Management Layer**
The data management layer provides an abstraction over data storage and files, providing a unified API for the upper layers. Contains two key components: **DataManager** for working with structured data (relational database) and **FileManager** for working with files.

###### **DataManager**
A central data management component that provides local database access and reactive UI updating via the Observer pattern.

**Responsibility**:
- Resource management (downloading, updating, deleting), working with the local database.
- Reception of CRUD operations from the network layer through AppManager and their processing (implementation)
- Managing UI component subscriptions to data changes (notifications)
- Reset all data when performing FULL_SYNC on a command from AppManager, subsequently passive CRUD operations

**Three types of subscriptions**:
1. **ResourceListObserver** - for a list of resources (for generative subsystems)
2. **ResourceObserver** - to a specific resource by ID (for dynamic widgets)
3. **TreeObserver** - on a tree structure (for hierarchical data)

Subscription mechanism via SubscribeAdaptor:
To avoid synchronization problems, dangling pointers, broadcast notifications and direct calls between DataManager and Observers, the SubscribeAdaptor adapter class is used.

###### **FileManager**
Coordinator of all file operations, providing centralized management of uploading, downloading and caching of files from MinIO.

Responsibility:
- Coordination of the chain of operations for retrieving a file - from a **presignedUrl** request from the server to a file request to MinIO.
- File integrity check via SHA256 hash (taken from DataManager)
- Managing subscribers via IFileObserver (notification of completed download/upload).
- 


###### 4. **Business logic layer**
The business logic layer contains components that implement complex domain-specific operations that are decomposed into many network requests or local actions. This layer sits between the Data Layer and the UI Layer, using the DataManager and FileManager to access data, but encapsulating complex processing logic.

###### ****PDMManager****
Product Data Management Manager is a subsystem for managing design documentation and products. Performs complex tasks of restoring shapshot versions of projects, applying/undoing deltas.

Responsibility:
- Interaction with FileManager DataManager AppManager UI (providing API)
- Parsing and generation of XML snapshots of design documentation (CD)
- Product version management (Part, Assembly, Project)
- Processing delta changes (incremental snapshot updates)
- Validation of design documentation for compliance with ESKD standards
- Linking products with drawings (PDF files in MinIO)

###### ****ERPManager****
The materials and production analytics subsystem works locally and calculates shortages/forecasts using data on projects, purchases, etc.

Responsibility:
- Calculation of material requirements based on production plans
- Forecasting material shortages
- Analysis of available stocks vs required quantities
- Creation of reports on material balance


###### 5. **Visualization Layer (UI Layer)**
**Components**: MainWindow, static/generative/dynamic widgets

**Widget structure**:

**Static widgets**:
- MainWindow - orchestration of all other widgets
- AuthWidget - authentication widget
- WorkspaceWidget - work area with tabs
**Responsibility**:
- formation of basic control of the application by the user

**Subsystem widgets**:
**Responsibility**:
- Subscription to resources of a specific subsystem (list, tree)
- Opportunities for working with a specific business process
- Displaying data on a business process
- Processing user input (clicks, editing)
- Creating and managing dynamic widgets to work with specific resources

**Dynamic widgets**:
**Responsibility**:
- Subscribe to ResourceObserver to receive updates for a specific resource
- Processing user input (clicks, editing)
- Subscribe to DataManager to receive data
- Formation of UniterMessage during CRUD actions
- Created by generative tabs



##### **3. Subsystems**

Each subsystem owns its own resources, which it manages using CRUD operations. Resources/subsystems can reference resources from another subsystem using id. Each resource is assigned a unique id, distributed centrally. Each host stores all data about all resources, and the server only switches CRUD operations.

**Material database subsystem**
Manages material templates MaterialTemplate, which reflects a specific standard and its possible variants, serves as the basis for introducing a MaterialInstanse (an independent entity that allows you to point to a standard and specialize it). Inside the MaterialTemplate there are vectors for linking them (when the material is denoted by a fraction - assortment/material). Also owns the MaterialTemplateComplex resources - this is a pair of MaterialTemplates, which can also be the basis for a MaterialInstanseComplex.

**Subsystem of links to materials**
Here you manage resources that are directly related to the previous section. Manages its own MaterialInstance resources that specialize a specific MaterialTemplate and add Quantity to it. As a result, an “instance” of material is formed, which can be referenced, for example, a purchase, or a part.

**Constructor subsystem**
Manages project projects (an independent resource with its own id), which internally contain assemblies, parts and their documentation. All these entities refer to MaterialInstance - a specialization of the material template.

**PDM subsystem**
It is managed by a snapshot (an independent resource with its own id), which inside itself contains a snapshot of a project, and deltas, which are a resource within a specific snapshot. The subsystem runs on top of the designer subsystem and allows you to introduce version control of design projects, propose changes, and accept or reject them.

**Procurement subsystem**
Manages ProcurementRequests that contain a MaterialInstance and complex ProcurementRequestComplex requests that contain references to simple purchases.

**Manager subsystem**
Manages special resources - employees, production, integration, in the same way using CRUD operations. At the same time, industries are generative resources, i.e. after the creation of production, a subsystem of this production appears, which is controlled through id, and manages resources within its subsystem.

**Generative production subsystem X**
Manages the ProdactionTask resource (a separate resource with its own id) and also contains the MaterialInstance list. (PS: can query data from the design subsystem and the purchasing subsystem, which allows the raid to calculate material shortages).

**Generative Integration Engine X**
Manages resource - integrations that allow you to open access to resources (primarily projects) for other companies. Uses references to design projects and materials. It is implemented using a gateway on the server, when passing through which the UniterMessage message is translated from one resource id space to another, thanks to the gateway correspondence table. The server keeps track of what is found for the resource id to which the CRUD operation is applied in this table, and if it is there, it switches the message. We are not implementing it yet.


##### **4. Development of UniterMessage**

The UniterMessage serves as a universal container for exchanging data between system components in three main scenarios:
1. **CRUD operations between subsystems** - a user on one host creates/changes/deletes a resource (for example, a new purchase item), the server distributes this change to all clients of the company via Kafka
2. **Protocol operations with the server** - authentication, request for full synchronization (FULL_SYNC), obtaining credentials for Kafka and tokens for MinIO
3. **Operations with file storage** - designed for MinIOConnector and performing requests for uploading/downloading files

UniterMessage is a generic data container that **does not contain specific structures for each resource type**. Instead of rigid typing, a flexible hierarchical data structure is used via XML payload, which makes it easy to expand the system with new types of resources without changing the protocol

###### **UniterMessage structure**

**MessageType** - message type, determines which additional fields are used:
- `CRUD` - operations of creating/reading/updating/deleting resources
- `PROTOCOL` - protocol operations with the server
- `MINIO` - operations with file storage

**MessageStatus** — message status in the request-response chain:
- `REQUEST` - request from the client (outgoing message)
- `RESPONSE` - response from the server (incoming message)
- `ERROR` - error when processing the request
- `NOTIFICATION` - notification about data changes by another user (from Kafka)
- `SUCCESS` - confirmation of the successful completion of its own CUD operation (from Kafka)

###### **Fields for CRUD operations (MessageType = CRUD)**

**Subsystem** is the subsystem to which the resource and specific **ResourceType** belong:
- `MATERIALS` - materials management
- `INSTANCES` - management of material elements
- `DESIGN` - design documentation
- `PDM` - versioning management
- `PURCHASES` - purchases and deliveries
- `MANAGER` - enterprise management
- `GENERATIVE` - production subsystems

**CrudAction** - operation type:

- `CREATE` - creating a new resource
- `UPDATE` - updating an existing resource
- `DELETE` - deleting a resource
- `READ` - reading a resource (not used in the current architecture, all readings come from the local database)

And also some general fields, such as user_id, creation time, payload for transferring some data (such as login/password).


###### **DB schema migrations**

DB schema migrations should not be treated as a local `migrations.sql` file within the subsystem. The target model is a separate migration resource in the new service subsystem, which is distributed by the server via `UniterMessage` in the same way as the rest of the system's managed data. The `PROTOCOL` subsystem is not used for this: it is responsible for control communication operations and application state, and not for storing and versioning the database structure.

The migration must have its own resource type, version/sequence number, target subsystem, SQL statement set or reference to the migration artifact, checksum, and application overhead. The client receives such a resource through a normal network route, stores the receipt, and then applies the migration to the local database in a controlled manner.

The application policy is currently fixed as an open architectural solution:
- migration can be applied immediately after receiving from the network if the client is in a state where it is safe to do so;
- or migrations can first be accumulated as resources, after which the server allows their use with a separate protocol command.

In both options, the use of migrations must be idempotent, transactional and verifiable through the state of the local database. Regular CRUD messages should only be processed after the required schema version has already been applied.


**Company merger**
When creating an integration (confirmation on both sides), the server registers this. Integration, like other resources, is assigned its own id, each unique for a specific company. After this, the server creates a space gateway id, initially empty. This is just a table of correspondence between one id and another company for all resources. When a special flag is set in UniterMessage, the required resources (primarily the project) will be entered into the gateway. Plus a complex algorithm for resolving other resources that the project refers to. After this, when switching messages, it will be checked whether these resources are marked in the gateway and transferred to the space of another company.


##### **5. Structures for describing resources**

Each shared resource (materials database entry, purchase, project, something else) also has a unique id within its type. Obtaining a new id is also managed centrally, through the server.

In this case, a link to a shared resource is simply an indication of its id and resource type. But when a resource is created or updated, then it is necessary to transmit complete information.

Resource update - no delta, always final result, providing additional reliability for reuse. Resource mapping is decided at the level of resource data structures, not at the message level. The message with the highest id wins, each client has a message queue, it applies and confirms only the next id.

The final list of resources to which CRUD is applicable. Each resource is managed only from its own subsystem; from other subsystems they are used through a link - just specifying the id.


###### **Materials**
The material database subsystem manages material templates. The material template models the standard, has vectors of suffix and prefix segments, which are denoted by id. Each segment has its own set of values, which also have their own ids. Removing and adding new segments does not break the structures that, through id, point to segments and their values, because each has its own unique id in its area of ​​application. The template also contains vectors that determine the order in which the segments are applied.

enum class SegmentValueType : uint8_t { 
	
STRING = 0, // Arbitrary string
ENUM = 1, // Select from the list
NUMBER = 2, // Number (int or double)
CODE = 3 // Code (special format)
};

// segment id makes sense ONLY within a specific MaterialTemplate (GOST),
// to which it belongs. In other contexts this id has no meaning.

struct SegmentDefinition { 
	
    uint8_t id;
QString code; // Machine name (for logic)
QString name; // Human readable name
QString description; // Description of the segment
SegmentValueType value_type;//Value type
std::map<uint8_t, std::string> allowed_values; // Segment status
bool is_active = true; // true = active, false = deleted (deprecated)
};

enum class GostStandardType : uint8_t { 
	
GOST = 0, // GOST
OST = 1, // OST
GOST_R = 2, // GOST R
TU = 3, // TU (technical condition)
SNIP = 4, // SNiP
OTHER = 5 // Other
};

enum class DimensionType : uint8_t { 
PIECE = 0, // Piece
LINEAR = 1, // Linear (m)
AREA = 2 // Flat (m²)
};

enum class GostSource : uint8_t { 
BUILT_IN = 0, // Preset standard
COMPANY_SPECIFIC = 1 // Company specific
};

class MaterialTemplateBase : public ResourceAbstract { 
public: 
	virtual ~MaterialTemplateBase() = default;
	
QString name; // Name
QString description; // Detailed description, scope
DimensionType dimension_type; // Material dimension
bool is_standalone; // Can this GOST independently describe the product

// Template metadata
GostSource source; // Source: built-in or company-specific

// Virtual methods to distinguish between types
    virtual bool isComposite() const = 0;

// Serialization deserialization
    virtual void from_xml(tinyxml2::XMLElement* source) = 0;
    virtual void to_xml(tinyxml2::XMLElement* dest) = 0;
};

Based on the basic template, two successors are constructed:

class MaterialTemplateSimple : public MaterialTemplateBase { 
public: 
	
GostStandardType standard_type; // Standard type
QString standard_number; // Standard number
QString year; // Standard version
	
// Standard prefixes
    std::map<uint8_t, SegmentDefinition> prefix_segments;
std::vector<uint8_t> prefix_order; // Order of prefixes
	
// Standard suffixes
    std::map<uint8_t, SegmentDefinition> suffix_segments;
std::vector<uint8_t> suffix_order; // Order of suffixes
	
    bool isComposite() const override { return false; }
	
// Serialization deserialization
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};

// A compound template references two simple ones
class MaterialTemplateComposite : public MaterialTemplateBase { 
public: 
	
QString PrefName; // Write before the standard (leaf, circle...)
uint64_t top_template_id; // Simple template ID for the top part
uint64_t bottom_template_id; // Simple template ID for the bottom
	
    bool isComposite() const override { return true; }
	
// Serialization deserialization
    virtual void from_xml(tinyxml2::XMLElement* source) override;
    virtual void to_xml(tinyxml2::XMLElement* dest) override;
};


#### **6. Links to materials (MaterialInstance)**

To indicate a specific type of material (which is a combination of a template reference and a specialized prefix/suffix with specific segment values), other subsystems use the MaterialInstance class, which is also inherited for the regular and composite material classes

// Size/quantity depends on dimension_type
struct figure
{
    double area = 0;
};
struct Quantity {
    std::optional"<"int> items;
    std::optional"<"double> length;
    std::optional"<"figure> fig;
};

class MaterialInstanceBase { 
public: 
    MaterialInstanceBase() {}
virtual ~MaterialInstanceBase() = default; // Identification

uint64_t template_id; // Link to MaterialTemplateBase (GOST)
QString name; // Human readable name of the material
QString description; // Description
DimensionType dimension_type; // Copied from template for quick access
    Quantity quantity;

// Virtual methods
    virtual bool isComposite() const = 0;

// Serialization deserialization
    virtual void from_xml(tinyxml2::XMLElement* source) = 0;
    virtual void to_xml(tinyxml2::XMLElement* dest) = 0;
};

class MaterialInstanceSimple : public MaterialInstanceBase {
public:

std::map<uint8_t, std::string> prefix_values; // Prefix values
std::map<uint8_t, std::string> suffix_values; // Suffix values
    bool isComposite() const override { return false; }
    void setPrefixValue(uint8_t segment_id, const uint8_t value);
    void setSuffixValue(uint8_t segment_id, const uint8_t value);
    std::optional<std::string> getPrefixValue(uint8_t segment_id) const;
    std::optional<std::string> getSuffixValue(uint8_t segment_id) const;

// Serialization deserialization
    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};


class MaterialInstanceComposite : public MaterialInstanceBase {
public:
// Filled segment values ​​for composite material
std::map<uint8_t, std::string> top_values; // Top values
std::map<uint8_t, std::string> bottom_values; // Bottom values

    bool isComposite() const override { return true; }

    void from_xml(tinyxml2::XMLElement* source) override;
    void to_xml(tinyxml2::XMLElement* dest) override;
};
#### **7. Runtime order**

1. Creating the main window (static widgets). This sets the base state
2. Creating a data manager, linking signals/slots to the main window
3. Creating an application manager that creates a configuration manager and local settings. Linking signals/slots to the main window and data manager.
4. Creating a network class, associating signals/slots with the application manager.
5. Launching the application manager (a short call to a function that will only send a signal and return to the signal processing loop). At the same time, it processes the initialization FSM: it sends a signal to the network to start the connection, and the main thread enters the message processing loop.

6. Network initialization
1. The network class gets the server IP via DNS
2. Establishes an SSL connection and then sends the signalConnected signal.
7. Authorization and configuration
1. The application manager requests authorization data from the authorization widget and sends the request to the server
2. If authorization is successful, the server sends resources::User.
3. The application manager initiates the data manager, and the data is loaded.
4. The application manager passes the User to the configuration manager, which serves as the basis for generating tab widgets
5. The application manager launches the local settings manager, which applies local settings, associates local folders and projects, etc.
6. The application enters the READY state.
7. At the same time, checking and downloading updates is initiated
8. Creating CRUD operations by the user and receiving messages
1. UniterMessage is generated and sent to the server
2. After processing by the server, a UniterMessage is sent
3. Only after receiving the UniterMessage are changes made to the local database and the view changed.
9. Creating new generative tabs
1. If a CRUD operation affects resources that are responsible for generative subsystems, the server processes it as a regular resource.
2. When adding a generative tab for Users, the server transmits the new User to clients.
10. Handling User Change
1. Receiving a new User is processed by the application manager like other protocol UniterMessages, and is transferred to the configuration manager
 


#### **8. Connecting signals and slots**

Dynamic widgets and subsystem widgets use IDataObserver to receive data from the local database, subscribe to some resources, and IFileObserver to access files from the MinIO server.

Also, subsystem tabs use a connection with AppManager to send messages, and dynamic widgets use a connection to their subsystem to send messages through it.

Most of the objects that exist throughout the entire program are implemented as singletons, while connecting in main() to emphasize the incorrectness of arbitrary connections in runtime, except for those specifically designated (only sending messages for AppManager and auto-connections when creating an observer).

#### **9. About PLM**
At the entrance we have a directory with design files. After parsing, we convert these files into structured XML form. If parsing is performed for the first time, then a new shapshot resource is created. If the second, then a delta snapshot is created based on parsing (more on this below)

The Snapshot XML structure separates **links** (product structure) and **definitions** (item parameters). Inside each assembly there are two elements **structure \<structure>** and **parts \<parts>** used directly in this assembly. Inside the structure there is a **constant data \<invariant>** element, as well as variable data for each **configuration \<config>**, which together provide complete data about the structure for each configuration. In this case, only references to parts/assemblies are used in the structure, for example \<partref> \<assemblyref>. Complete parts data is contained within \<parts>.  This eliminates data duplication and allows parts to be reused in different assemblies.


	<shapshot id="" designation="" name="" version="2" previousVersion="1">
	
	  <Assembly designation="" name="">
	    <Metadata>...</Metadata>
	    <Documentation>
	      <File path="..." hash="sha256:..." modifiedAt="..."/>
	    </Documentation>
	    <Structure>
	      <Invariant>
	        <Assemblies><AssemblyRef designation="" config=""/></Assemblies>
	        <Parts><PartRef designation="" config=""/></Parts>
	        <StandardProducts/>
	        <BuyingProducts/>
	        <OtherMaterials/>
	      </Invariant>
	      <Config number="01">...</Config>
	      <Config number="02">...</Config>
	    </Structure>
	    <PartsDef>
	      <Part designation="" name="">
	        <Config id="01"/>
	        <Config id="02"/>
	      </Part>
	    </PartsDef>
	  </Assembly>
	
	  <Errors>
	    <Error severity="Error" category="FileSystem" type="NO_FILE"
	           path="drawings/4021.01.00.02.pdf"/>
	    <Error severity="Warning" category="VersionControl" type="INFORMAL_CHANGE"
	           designation="4021.01.00.03" hashBefore="sha256:a1b2..." hashAfter="sha256:c3d4..."/>
	  </Errors>
	
	</Shapshot>


And the partdef element is simply a container for MaterialInstance + other meta information.


<Partdef designation="4021.01.00.01" name="Longitudinal beam">
	  <Metadata>
	    <Material>
<Base>Channel 20P GOST 8239-89</Base>
	      <CoatingTop/>
	      <CoatingBottom/>
	    </Material>
	    <Signatures>
<Signature role="Developed" name="Ivanov I.P." date="2026-01-15"/>
<Signature role="Checked" name="Petrov S.A."  date="2026-01-18"/>
	    </Signatures>
<Litera>O</Litera>
<Organization>OJSC "Plant"</Organization>
	    <DrawingFile>
<Path>drawings/4021.01.00.01_rev.A.pdf</Path>
	      <Hash>sha256:e7f8g9...</Hash>
	      <ModifiedAt>2026-01-15T11:30:00+03:00</ModifiedAt>
	    </DrawingFile>
	  </Metadata>
	  <Config id="01">
	    <Dimensions length="2400" width="200" height="80"/>
	    <Mass>85.2</Mass>
	  </Config>
	  <Config id="02">
	    <Dimensions length="2800" width="200" height="80"/>
	    <Mass>99.4</Mass>
	  </Config>
	</Partdef>


In this case, based on the parsing results, separate management of the resources of the designer subsystem and separate management of the resources of the PDM subsystem are performed. The PDM subsystem owns its own resources - project snapshots and deltas within them, and allows you to manage changes independently, incl. roll back to previous versions, propose changes, accept or reject them.

###### **Version control (within XML delta formation)**

To manage versioning, an approach is used to logically separate the criteria for changes from the data on the basis of which the decision about the presence of changes is made. The data on the basis of which the decision on the presence of changes is made is collected independently, to the maximum extent, for all versioning criteria. This data is collected during parsing.

Document change criteria are introduced - this means that simply changing the hash will not be a sufficient basis for recording changes in XML delta. To record changes, one of the change criteria must be met (in addition to the obvious additions of configurations, or changes in dimensions, a change in the file name is also assessed - for example, Rev.B at the end, or adding information to the title block).



#### **10. Architecture Client-Server-Kafka-Postgres-MinIO**

All key information is stored in a relational database (both on the server and locally). All heavy files are stored in the MinIO cloud, and if a resource requires such a file, it stores the URL to them inside the database. In this case, the client can request temporary access to files via UniterMessage from the server, and also upload files there independently, for example, when creating resources. Fields for URL requests have not yet been processed in UniterMessage.


#### **11. Drawing Compiler**

The compilation algorithm should look like this.

First, ideally we need to get a list of files that belong to the project. But we can also identify them ourselves by the project name and file names. So, we have created an initiating list of pdf files.

Further, because each pdf can contain several drawings, we form an initiating list of translation primitives - each separate pdf page, with a saved path to it and context.

Next, we need to determine the type of each translation primitive (specification, assembly drawing, part, installation drawing, and we need to distinguish the first page from the others). To do this, we extract the designation for each page. It works to our advantage that each first page contains a title block of the same size in the lower right corner, as well as a designation in the upper left corner, and the following pages of the same document have the same designation, only they also have a sheet entry other than “1”.

After this, we transform our list from translation primitives into translation units, logically combining documents. Next, translation units are distributed among buckets using type-specific compilers.

After passing through type-specific compilers, the project is already linked into a tree.


Important! The creation of an XMLDocument, as well as the root element with its attributes, is performed at the PDMManager level, and only the root is passed to the compilation library. In this case, it is PDMManager that manages document saving, etc.
