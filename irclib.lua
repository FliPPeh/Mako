----
-- IRC API wrappers
--
-- The API between the bot and plugins is based on mostly
-- atomic types and simple functions. These classes make
-- their usage a bit easier.
----
local irc = {}

----
-- Generic IRC target (messageable, respondable)
----
irc.generic_target = {}

function irc.generic_target:privmsg(msg)
    bot.sendmsg{
        command = 'PRIVMSG',
        args = {
            self:target()
        },
        message = msg
    }
end

function irc.generic_target:notice(msg)
    bot.sendmsg{
        command = 'NOTICE',
        args = {
            self:target()
        },
        message = msg
    }
end


function irc.generic_target:respond(msg)
    self:privmsg(msg)
end

----
-- Any kind of message source (prefix)
----
irc.source = setmetatable({}, {
    __index = irc.generic_target
})

irc.source_meta = { __index = irc.source }

function irc.source.new(prefix)
    return setmetatable({
        prefix = prefix
    }, irc.source_meta)
end

function irc.source:is_user()
    return self.prefix:find('!') and self.prefix:find('@')
end

function irc.source:nick()
    if self:is_user() then
        return self.prefix:sub(0, self.prefix:find('!') - 1)
    end
end

function irc.source:user()
    if self:is_user() then
        return self.prefix:sub(
            self.prefix:find('!') + 1, self.prefix:find('@') - 1)
    end
end

function irc.source:host()
    if self:is_user() then
        return self.prefix:sub(self.prefix:find('@') + 1)
    else
        return self.prefix
    end
end

function irc.source:target()
    return self:nick() or self:host()
end

function irc.source:__to_channel_user(channel)
    if channel:users()[self.prefix] then
        return irc.channel_user.new(channel, self.prefix)
    else
        error(string.format('%q is not in channel %q',
            self.prefix, channel.name), 2)
    end
end


function irc.source_meta:__tostring()
    return self:nick() or self:host()
end

----
-- An IRC channel
----
irc.channel = setmetatable({}, {
    __index = irc.generic_target
})

irc.channel_meta = { __index = irc.channel }

function irc.channel.new(chan)
    for i, channel in ipairs(bot.get_channels()) do
        if channel == chan then
            return setmetatable({ name = assert(chan) }, irc.channel_meta)
        end
    end

    error(string.format('no such channel %q', chan), 2)
end

function irc.channel:target()
    return self.name
end

function irc.channel:topic()
    meta = bot.get_channel_meta(self.name)

    return meta.topic, meta.topic_setter, meta.topic_set
end

function irc.channel:created()
    return bot.get_channel_meta(self.name).created
end

function irc.channel:users()
    return bot.get_channel_users(self.name)
end

function irc.channel:find_user(target)
    local nick = (target:find('!') and target:find('@'))
        and irc.source.new(target):nick()
        or target

    for prefix, _ in pairs(self:users()) do
        if irc.source.new(prefix):nick():lower() == nick:lower() then
            return irc.channel_user.new(self, prefix)
        end
    end

    error(string.format('no such user %q in channel  %q', nick, self.name), 2)
end

function irc.channel:try_find_user(target)
    local ok, ret = pcall(self.find_user, self, target)

    return ok and ret or nil
end

function irc.channel_meta:__tostring()
    return self.name
end


----
-- An IRC user with an associated channel
----
irc.channel_user = setmetatable({}, {
    __index = irc.source
})

irc.channel_user_meta = { __index = irc.channel_user }

function irc.channel_user.new(channel, prefix)
    assert(getmetatable(channel) == irc.channel_meta,
        'argument #1 must be of type "irc.channel_meta"')

    assert(prefix:find('!') and prefix:find('@'),
        'argument #2 must be a valid user prefix')

    return setmetatable({
        prefix    = prefix,
        channel = channel
    }, irc.channel_user_meta)
end

function irc.channel_user:respond(msg)
    self.channel:privmsg(string.format('%s: %s', self:nick(), msg))
end

function irc.channel_user:modes()
    return self.channel:users()[self.prefix]
end

function irc.channel_user_meta:__tostring()
    return self:nick()
end

return irc
