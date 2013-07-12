function is_array(t)
    local nkeys = 0
    for k, v in pairs(t) do
        nkeys = nkeys + 1
    end

    local nidx = 0
    for i, v in ipairs(t) do
        nidx = nidx + 1
    end

    return nkeys == nidx
end

function make_dumper(args)
    local indent_width = args.indent_width or 3
    local indent_char = args.indent_char or ' '
    local indent_level = 0
    local depth = args.depth or math.huge
    local inline = args.inline or false

    local indent = ''


    local function dumper(v)
        if type(v) == 'number' or type(v) == 'boolean' then
            return tostring(v)
        elseif type(v) == 'string' then
            return string.format('%q', v)
        elseif type(v) == 'table' then
            local list = is_array(v)

            depth = depth - 1
            if inline then
                local dump = '{ '

                if depth > 0 then
                    for k, v in pairs(v) do
                        if list then
                            dump = dump .. string.format('%s, ', dumper(v))
                        else
                            dump = dump .. string.format('%s = %s, ',
                                k, dumper(v))
                        end
                    end
                else
                    dump = dump .. '... '
                end

                return dump .. '}'
            else
                local dump = '{\n'

                indent_level = indent_level + 1
                indent = string.rep(indent_char, indent_width * indent_level)

                if depth > 0 then
                    for k, v in pairs(v) do
                        if list then
                            dump = dump .. string.format('%s%s,\n',
                                indent, dumper(v))
                        else
                            dump = dump .. string.format('%s%s = %s,\n',
                                indent, k, dumper(v))
                        end
                    end
                else
                    dump = dump .. indent .. '...\n'
                end

                indent_level = indent_level - 1
                indent = string.rep(indent_char, indent_width * indent_level)

                return dump .. indent .. '}'
            end
        end
    end

    return dumper
end

file_serializer = {}
file_serializer_meta = { __index = file_serializer }

function file_serializer.new(file)
    return setmetatable({ file = file }, file_serializer_meta)
end

function file_serializer:write(v)
    d = make_dumper{inline = false}
    f = io.open(self.file, 'w')
    f:write('return ' .. d(v) .. '\n')
    f:close()
end

function file_serializer:read()
    ok, ret = pcall(dofile, self.file)
    if ok then
        return ret
    else
        error(ret)
    end
end

-- API table
irc = {}

----
-- Generic IRC target (messageable, respondable)
----
irc.generic_target = {}

function irc.generic_target:privmsg(msg)
    bot.sendmsg{
        command = 'PRIVMSG', args = { self:target() }, message = msg
    }
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

    error(string.format('no such channel %q', channel), 2)
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

----
-- An IRC user, not associated with a channel
----
irc.user = setmetatable({}, {
    __index = irc.generic_target
})

irc.user_meta = { __index = irc.user }

function irc.user.new(prefix)
    assert(prefix)

    if not prefix:find('!') and not prefix:find('@') then
        error(string.format('not a valid prefix "%q"', prefix), 2)
    else
        return setmetatable({ prefix = prefix }, irc.user_meta)
    end
end

function irc.user:target()
    return self:nick()
end

function irc.user:nick()
    return self.prefix:sub(0, self.prefix:find('!') - 1)
end

function irc.user:user()
    return self.prefix:sub(self.prefix:find('!') + 1, self.prefix:find('@') - 1)
end

function irc.user:host()
    return self.prefix:sub(self.prefix:find('@') + 1)
end


----
-- An IRC user with an associated channel
----
irc.channel_user = setmetatable({}, {
    __index = irc.generic_target
})

irc.channel_user_meta = { __index = irc.channel_user }

function irc.channel_user.new(channel, prefix)
    return setmetatable({
        user    = irc.user.new(prefix),
        channel = irc.channel.new(channel)
    }, irc.channel_user_meta)
end

function irc.channel_user:respond(msg)
    self.channel:privmsg(string.format('%s: %s', self.user:nick(), msg))
end

----
-- Utils
----
logger = log.get_logger('bootstrap')
function dump_channels()
    local log = logger:derive('dump_channels')

    for i, channel in ipairs(bot.irc.get_channels()) do
        log:log('debug', channel.name)
        log:log('debug', string.format('Topic: %q %d (%s)',
        channel.topic,
        channel.topic_set,
        channel.topic_setter))

        log:log('debug', string.format('Created: %d', channel.created))

        for j, user in ipairs(channel.users) do
            log:log('debug', string.format(' -> %q (%s)',
            user.prefix,
            user.modes))
        end

        for mode, args in pairs(channel.modes) do
            log:log('debug', string.format('Mode: %q', mode))

            if type(args) == 'string' then
                log:log('debug', string.format(' Arg: %q', args))
            elseif type(args) == 'table' then
                for i, arg in ipairs(args) do
                    log:log('debug', string.format(' Arg %d: %q', i, arg))
                end
            end
        end
    end
end

function __on_event(ev) end
function __on_ping() end
function __on_privmsg(prefix, target, msg) end
function __on_notice(prefix, target, msg) end
function __on_join(prefix, chan) end
function __on_part(prefix, chan, reason) end
function __on_quit(prefix, reason) end
function __on_kick(prefix_kicker, prefix_kicked, chan, reason) end
function __on_nick(prefix_old, prefix_new) end
function __on_invite(prefix, chan) end
function __on_topic(prefix, channel, topic_old, topic_new) end
function __on_mode_set(prefix, chan, mode, arg) end
function __on_mode_unset(prefix, chan, mode, arg) end
function __on_modes(prefix, chan, modes) end
function __on_idle() end
function __on_connect()
    logger:log('info', 'Connecting...')

    bot.sendmsg({
        command = 'JOIN', args = { '#techbot' }
    })
end
function __on_disconnect() end
function __on_ctcp(prefix, target, ctcp, args) end
function __on_ctcp_response(prefix, target, ctcp, args) end
function __on_action(prefix, target, msg) end

function __on_command(prefix, target, command, args)
    if command == 'dump' then
        d = make_dumper{ inline = true }

        cu = irc.channel_user.new(target, prefix)
        cu:respond(d(cu.channel:users()))
    end
end
