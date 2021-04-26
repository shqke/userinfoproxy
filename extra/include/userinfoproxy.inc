#if defined __ext_userinfoproxy_included
 #endinput
#endif
#define __ext_userinfoproxy_included

#define USERINFOPROXY_LIB_NAME          "userinfoproxy"

#define USERINFOFLAG_NONE               0x00

#define USERINFOFLAG_SANITIZE_NAME      0x01 // Replace target name with "User #<userid>" (if forward took no action)
#define USERINFOFLAG_HIDE_AVATAR        0x02 // Replace avatar with a blank space
#define USERINFOFLAG_QUERY_FORWARD      0x04 // Query forward UserInfoExt_OnNameQuery for name

methodmap UserInfoExt
{
    /**
     * Returns override flags for a target player against single recipient or everyone.
     *
     * @param target        Index of userinfo data owner.
     * @param recipient     Index of player receiving overridden userinfo data, -1 for everyone.
     * @param sum           If true will return sum of flags from all plugins, setting of a calling plugin otherwise.
     * @return              USERINFOFLAG_* bitstring containing which flags are enabled.
     * @error               If the client is not connected an error will be thrown.
     */
    public static native int GetOverride(int target, int recipient = INVALID_ENT_REFERENCE, bool sum = false);

    /**
     * Sets override flags for a target player against single recipient or everyone.
     * @note Plugin settings are reset on plugin unload.
     * @note Client settings are reset on client disconnect (including map change).
     *
     * @param target        Index of userinfo data owner.
     * @param flags         USERINFOFLAG_* bitstring containing which flags are enabled.
     * @param recipient     Index of player receiving overridden userinfo data, -1 for everyone.
     * @error               If the client is not connected an error will be thrown.
     */
    public static native void SetOverride(int target, int flags, int recipient = INVALID_ENT_REFERENCE);
    
    /**
     * Resets override flags for a target player against single recipient or everyone.
     *
     * @param target        Index of userinfo data owner.
     * @param recipient     Index of player receiving overridden userinfo data, -1 for everyone.
     * @error               If the client is not connected an error will be thrown.
     */
    public static void ResetOverride(int target, int recipient = INVALID_ENT_REFERENCE)
    {
        SetOverride(target, USERINFOFLAG_NONE, recipient);
    }
};

/**
 * Called if override has USERINFOFLAG_QUERY_FORWARD flag.
 *
 * @param target        Index of userinfo data owner.
 * @param recipient     Index of player receiving overridden userinfo data.
 * @param name          Buffer to store the name of the target.
 * @param maxlength     Maximum length of string buffer.
 * @return              Plugin_Changed to change name, Plugin_Stop to change and halt, Plugin_Continue to take no action.
 */
forward Action UserInfoExt_OnNameQuery(int target, int recipient, char[] name, int maxlength);

public Extension __ext_userinfoproxy =
{
    name = "userinfo proxy",
    file = "userinfoproxy.ext",
#if defined AUTOLOAD_EXTENSIONS
    autoload = 1,
#else
    autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
    required = 1,
#else
    required = 0,
#endif
};

public __ext_userinfoproxy_SetNTVOptional()
{
    MarkNativeAsOptional("UserInfoExt.GetOverride");
    MarkNativeAsOptional("UserInfoExt.SetOverride");
}
