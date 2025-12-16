#ifndef UNETMESTAGS_H
#define UNETMESTAGS_H

struct UMTags {
    static constexpr const char* PROTOCOL = "PROTOCOL";
    static constexpr const char* UNETMES = "UNET-MES";
    static constexpr const char* NOUPDATE = "NOUPDATE";
    static constexpr const char* SOMEUPDATE = "SOMEUPDATE";
    static constexpr const char* GETUPDATE = "GETUPDATE";
    static constexpr const char* VERSION = "VERSION";
    static constexpr const char* HASH = "HASH";

    static constexpr const char* NEWDIR = "NEWDIR";
    static constexpr const char* NEWFILE = "NEWFILE";
    static constexpr const char* DELFILE = "DELFILE";
    static constexpr const char* DELDIR = "DELDIR";

    static constexpr const char* AGREE = "AGREE";
    static constexpr const char* REJECT = "REJECT";
    static constexpr const char* COMPLETE = "COMPLETE";
};

#endif // UNETMESTAGS_H
