#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * IRC commands and numerics
 */
#define IRC_COMMANDS \
    X(PASS)       /* <password> */                                            \
    X(NICK)       /* <nickname> */                                            \
    X(USER)       /* <user> <host> <server> <real> */                         \
    X(OPER)       /* <user> <password> */                                     \
    X(QUIT)       /* [<quit message>] */                                      \
    X(JOIN)       /* <channel>{,<channel>} [<key>{,<key>}] */                 \
    X(PART)       /* <channel>{,<channel>} */                                 \
    X(MODE)       /* <channel> <modestr> [{<arg>}] | <nickname> <modestr> */  \
    X(TOPIC)      /* <channel> [<topic>] */                                   \
    X(NAMES)      /* [<channel>{,<channel>}] */                               \
    X(LIST)       /* [<channel>{,<channel>} [<server>]] */                    \
    X(INVITE)     /* <nickname> <channel> */                                  \
    X(KICK)       /* <channel> <user> [<comment>] */                          \
    X(VERSION)    /* [<server>] */                                            \
    X(SERVER)     /* [<query> [<server>]] */                                  \
    X(LINKS)      /* [[<remote server>] <server mask>] */                     \
    X(TIME)       /* [<server>] */                                            \
    X(CONNECT)    /* <target server> [<port> [<remote server>]] */            \
    X(TRACE)      /* [<server>] */                                            \
    X(ADMIN)      /* [<server>] */                                            \
    X(INFO)       /* [<server>] */                                            \
    X(PRIVMSG)    /* <receiver>{,<receiver>} <text> */                        \
    X(NOTICE)     /* <nickname> <text> */                                     \
    X(WHO)        /* [<name> [<o>]] */                                        \
    X(WHOIS)      /* [<server>] <nickmask>[,<nickmask>[,...]] */              \
    X(WHOWAS)     /* <nickname> [<count> [<server>]] */                       \
    X(KILL)       /* <nickname> <comment> */                                  \
    X(PING)       /* <server1> [<server2>] */                                 \
    X(PONG)       /* <server1> [<server2>] */                                 \
    X(ERROR)      /* <error message> */                                       \
    X(AWAY)       /* [message] */

#define IRC_NUMERICS \
    X(RPL_WELCOME,            1) \
    X(RPL_YOURHOST,           2) \
    X(RPL_CREATED,            3) \
    X(RPL_MYINFO,             4) \
    X(RPL_ISUPPORT,           5) \
    X(RPL_REDIR,             10) \
    X(RPL_MAP,               15) \
    X(RPL_MAPMORE,           16) \
    X(RPL_MAPEND,            17) \
    X(RPL_YOURID,            20) \
    X(RPL_TRACELINK,        200) \
    X(RPL_TRACECONNECTING,  201) \
    X(RPL_TRACEHANDSHAKE,   202) \
    X(RPL_TRACEUNKNOWN,     203) \
    X(RPL_TRACEOPERATOR,    204) \
    X(RPL_TRACEUSER,        205) \
    X(RPL_TRACESERVER,      206) \
    X(RPL_TRACENEWTYPE,     208) \
    X(RPL_TRACECLASS,       209) \
    X(RPL_STATSLINKINFO,    211) \
    X(RPL_STATSCOMMANDS,    212) \
    X(RPL_STATSCLINE,       213) \
    X(RPL_STATSNLINE,       214) \
    X(RPL_STATSILINE,       215) \
    X(RPL_STATSKLINE,       216) \
    X(RPL_STATSQLINE,       217) \
    X(RPL_STATSYLINE,       218) \
    X(RPL_ENDOFSTATS,       219) \
    X(RPL_STATSPLINE,       220) \
    X(RPL_UMODEIS,          221) \
    X(RPL_STATSFLINE,       224) \
    X(RPL_STATSDLINE,       225) \
    X(RPL_STATSALINE,       226) \
    X(RPL_SERVLIST,         234) \
    X(RPL_SERVLISTEND,      235) \
    X(RPL_STATSLLINE,       241) \
    X(RPL_STATSUPTIME,      242) \
    X(RPL_STATSOLINE,       243) \
    X(RPL_STATSHLINE,       244) \
    X(RPL_STATSSLINE,       245) \
    X(RPL_STATSXLINE,       247) \
    X(RPL_STATSULINE,       248) \
    X(RPL_STATSDEBUG,       249) \
    X(RPL_STATSCONN,        250) \
    X(RPL_LUSERCLIENT,      251) \
    X(RPL_LUSEROP,          252) \
    X(RPL_LUSERUNKNOWN,     253) \
    X(RPL_LUSERCHANNELS,    254) \
    X(RPL_LUSERME,          255) \
    X(RPL_ADMINME,          256) \
    X(RPL_ADMINLOC1,        257) \
    X(RPL_ADMINLOC2,        258) \
    X(RPL_ADMINEMAIL,       259) \
    X(RPL_TRACELOG,         261) \
    X(RPL_ENDOFTRACE,       262) \
    X(RPL_LOAD2HI,          263) \
    X(RPL_LOCALUSERS,       265) \
    X(RPL_GLOBALUSERS,      266) \
    X(RPL_VCHANEXIST,       276) \
    X(RPL_VCHANLIST,        277) \
    X(RPL_VCHANHELP,        278) \
    X(RPL_ACCEPTLIST,       281) \
    X(RPL_ENDOFACCEPT,      282) \
    X(RPL_NONE,             300) \
    X(RPL_AWAY,             301) \
    X(RPL_USERHOST,         302) \
    X(RPL_ISON,             303) \
    X(RPL_TEXT,             304) \
    X(RPL_UNAWAY,           305) \
    X(RPL_NOWAWAY,          306) \
    X(RPL_USERIP,           307) \
    X(RPL_WHOISUSER,        311) \
    X(RPL_WHOISSERVER,      312) \
    X(RPL_WHOISOPERATOR,    313) \
    X(RPL_WHOWASUSER,       314) \
    X(RPL_ENDOFWHOWAS,      369) \
    X(RPL_WHOISCHANOP,      316) \
    X(RPL_WHOISIDLE,        317) \
    X(RPL_ENDOFWHOIS,       318) \
    X(RPL_WHOISCHANNELS,    319) \
    X(RPL_LISTSTART,        321) \
    X(RPL_LIST,             322) \
    X(RPL_LISTEND,          323) \
    X(RPL_CHANNELMODEIS,    324) \
    X(RPL_CREATIONTIME,     329) \
    X(RPL_NOTOPIC,          331) \
    X(RPL_TOPIC,            332) \
    X(RPL_TOPICWHOTIME,     333) \
    X(RPL_WHOISACTUALLY,    338) \
    X(RPL_INVITING,         341) \
    X(RPL_INVITELIST,       346) \
    X(RPL_ENDOFINVITELIST,  347) \
    X(RPL_EXCEPTLIST,       348) \
    X(RPL_ENDOFEXCEPTLIST,  349) \
    X(RPL_VERSION,          351) \
    X(RPL_WHOREPLY,         352) \
    X(RPL_ENDOFWHO,         315) \
    X(RPL_NAMREPLY,         353) \
    X(RPL_ENDOFNAMES,       366) \
    X(RPL_KILLDONE,         361) \
    X(RPL_CLOSING,          362) \
    X(RPL_CLOSEEND,         363) \
    X(RPL_LINKS,            364) \
    X(RPL_ENDOFLINKS,       365) \
    X(RPL_BANLIST,          367) \
    X(RPL_ENDOFBANLIST,     368) \
    X(RPL_INFO,             371) \
    X(RPL_MOTD,             372) \
    X(RPL_INFOSTART,        373) \
    X(RPL_ENDOFINFO,        374) \
    X(RPL_MOTDSTART,        375) \
    X(RPL_ENDOFMOTD,        376) \
    X(RPL_YOUREOPER,        381) \
    X(RPL_REHASHING,        382) \
    X(RPL_MYPORTIS,         384) \
    X(RPL_NOTOPERANYMORE,   385) \
    X(RPL_RSACHALLENGE,     386) \
    X(RPL_TIME,             391) \
    X(RPL_USERSSTART,       392) \
    X(RPL_USERS,            393) \
    X(RPL_ENDOFUSERS,       394) \
    X(RPL_NOUSERS,          395) \
    X(ERR_NOSUCHNICK,       401) \
    X(ERR_NOSUCHSERVER,     402) \
    X(ERR_NOSUCHCHANNEL,    403) \
    X(ERR_CANNOTSENDTOCHAN, 404) \
    X(ERR_TOOMANYCHANNELS,  405) \
    X(ERR_WASNOSUCHNICK,    406) \
    X(ERR_TOOMANYTARGETS,   407) \
    X(ERR_NOORIGIN,         409) \
    X(ERR_NORECIPIENT,      411) \
    X(ERR_NOTEXTTOSEND,     412) \
    X(ERR_NOTOPLEVEL,       413) \
    X(ERR_WILDTOPLEVEL,     414) \
    X(ERR_UNKNOWNCOMMAND,   421) \
    X(ERR_NOMOTD,           422) \
    X(ERR_NOADMININFO,      423) \
    X(ERR_FILEERROR,        424) \
    X(ERR_NONICKNAMEGIVEN,  431) \
    X(ERR_ERRONEUSNICKNAME, 432) \
    X(ERR_NICKNAMEINUSE,    433) \
    X(ERR_NICKCOLLISION,    436) \
    X(ERR_UNAVAILRESOURCE,  437) \
    X(ERR_NICKTOOFAST,      438) \
    X(ERR_USERNOTINCHANNEL, 441) \
    X(ERR_NOTONCHANNEL,     442) \
    X(ERR_USERONCHANNEL,    443) \
    X(ERR_NOLOGIN,          444) \
    X(ERR_SUMMONDISABLED,   445) \
    X(ERR_USERSDISABLED,    446) \
    X(ERR_NOTREGISTERED,    451) \
    X(ERR_ACCEPTFULL,       456) \
    X(ERR_ACCEPTEXIST,      457) \
    X(ERR_ACCEPTNOT,        458) \
    X(ERR_NEEDMOREPARAMS,   461) \
    X(ERR_ALREADYREGISTRED, 462) \
    X(ERR_NOPERMFORHOST,    463) \
    X(ERR_PASSWDMISMATCH,   464) \
    X(ERR_YOUREBANNEDCREEP, 465) \
    X(ERR_YOUWILLBEBANNED,  466) \
    X(ERR_KEYSET,           467) \
    X(ERR_CHANNELISFULL,    471) \
    X(ERR_UNKNOWNMODE,      472) \
    X(ERR_INVITEONLYCHAN,   473) \
    X(ERR_BANNEDFROMCHAN,   474) \
    X(ERR_BADCHANNELKEY,    475) \
    X(ERR_BADCHANMASK,      476) \
    X(ERR_MODELESS,         477) \
    X(ERR_BANLISTFULL,      478) \
    X(ERR_BADCHANNAME,      479) \
    X(ERR_NOPRIVILEGES,     481) \
    X(ERR_CHANOPRIVSNEEDED, 482) \
    X(ERR_CANTKILLSERVER,   483) \
    X(ERR_RESTRICTED,       484) \
    X(ERR_BANNEDNICK,       485) \
    X(ERR_NOOPERHOST,       491) \
    X(ERR_UMODEUNKNOWNFLAG, 501) \
    X(ERR_USERSDONTMATCH,   502) \
    X(ERR_GHOSTEDCLIENT,    503) \
    X(ERR_USERNOTONSERV,    504) \
    X(ERR_VCHANDISABLED,    506) \
    X(ERR_ALREADYONVCHAN,   507) \
    X(ERR_WRONGPONG,        513) \
    X(ERR_LONGMASK,         518) \
    X(ERR_HELPNOTFOUND,     524) \
    X(RPL_MODLIST,          702) \
    X(RPL_ENDOFMODLIST,     703) \
    X(RPL_HELPSTART,        704) \
    X(RPL_HELPTXT,          705) \
    X(RPL_ENDOFHELP,        706) \
    X(RPL_KNOCK,            710) \
    X(RPL_KNOCKDLVR,        711) \
    X(ERR_TOOMANYKNOCK,     712) \
    X(ERR_CHANOPEN,         713) \
    X(ERR_KNOCKONCHAN,      714) \
    X(ERR_KNOCKDISABLED,    715) \
    X(ERR_LAST_ERR_MSG,     999)

enum irc_command
{
#define X(num, n) num = n,
    IRC_NUMERICS
#undef X

#define X(txt) CMD_ ## txt,
    IRC_COMMANDS
#undef X
};

#endif /* defined PROTOCOL_H */
