irc = require 'irclib'
dump = require 'dumplib'

-- Two default dumpers
dumper = dump.make_dumper{ inline = false, depth = 8 }
idumper = dump.make_dumper{ inline = true, depth = 3 }

----
-- Testbed
----
if not bot then
    bot = {
        sendmsg = function(tab)
            print(dumper(tab))
        end,

        get_identity = function(kind)
            local kind = kind or 'n'

            local i = {
                nick = 'Derpl',
                user = 'derpl',
                realname = 'Mister Zorg requires shiny objects'
            }

            if kind == '*a' then
                return i.nick, i.user, i.realname
            elseif kind == 'n' then
                return i.nick
            elseif kind == 'u' then
                return i.user
            elseif kind == 'r' then
                return i.realname
            end
        end,

        get_channels = function()
            return { '#channel' }
        end,

        get_channel_users = function(chan)
            return ({
                ['#channel'] = {
                    ['FliPPeh!flippeh@flippeh.eu'] = "o"
                }
            })[chan] or error(string.format('no such channel %q', chan))
        end
    }
end

if not log then
    log = {
        __names = { 'mod_lua' }
    }

    function log.log(...)
        local name, lev, msg = nil, nil, nil

        if #{...} > 2 then
            name, lev, msg = ...
        else
            lev, msg = ...
        end

        if name then
            print(string.format('(%s) [%s] %s', name, lev, msg))
        else
            print(string.format('[%s] %s', lev, msg))
        end
    end
end

local log_log = log.log
function log.log(...)
    local name, lev, msg = nil, nil, nil

    if #{...} > 2 then
        name, lev, msg = ...
    else
        lev, msg = ...
    end

    log_log('mod_lua', lev, msg)
end

----
-- Utils
----
function try(fn, default)
    ok, ret = pcall(fn)
    return ok and ret or default
end

----
-- Signal registration and dispatching (conflicts with modules)
----
signal = {
    -- Weak-valued handlers table, so keeping bound handlers will not
    -- prevent an external module from being GC'd
    __handlers = {}
}

function signal.bind_multi(sigtype, handler)
    return signal.bind(sigtype, function(...)
        handler(sigtype, ...)
    end)
end

-- SAVE THE RETURN VALUE! It's the only reference that will keep the
-- handler from being GC'd if handler is an anonymous function.
--
-- This is so that signal handlers get unbound automatically when the module
-- that installed them gets GC'd.
--
-- Also, you need it to unbind the handler.
function signal.bind(sigtype, handler)
    if signal.__handlers[sigtype] then
        for i, ohandler in ipairs(signal.__handlers[sigtype]) do
            if ohandler == handler then
                error(string.format('handler already bound for signal %q',
                    sigtype), 2)
            end
        end

        table.insert(signal.__handlers[sigtype], handler)
    else
        signal.__handlers[sigtype] = setmetatable({ handler }, { __mode = 'v' })
    end

    return handler
end

function signal.unbind(sigtype, handler)
    for i, ohandler in ipairs(signal.__handlers[sigtype]) do
        if ohandler == handler then
            table.remove(signal.__handlers[sigtype], i)
            break
        end
    end
end

function signal.dispatch(sigtype, ...)
    log.log('trace', string.format('dispatching signal %q with args %s',
        sigtype, idumper{...}))

    if signal.__handlers[sigtype] then
        for i, handler in ipairs(signal.__handlers[sigtype]) do
            log.log('trace', string.format('call handler %s', handler))
            handler(...)
        end
    end
end

----
-- Modules (scripts)
----
modules = {
    __loaded = {}
}

function modules.load(modnam)
    if not package.loaded[modnam] then
        local mod = require(modnam)

        mod.__init()

        local module = {
            exports = setmetatable(mod, {
                __gc = function()
                    mod.__exit()
                end
            }),
            name = mod.__info.name,
            descr = mod.__info.descr
        }

        modules.__loaded[modnam] = module
        return module
    else
        error(string.format('module %q already loaded', modnam), 2)
    end
end

function modules.unload(modnam)
    if modules.__loaded[modnam] then
        local mod = modules.__loaded[modnam]

        modules.__loaded[modnam] = nil
        package.loaded[modnam] = nil
        _G[modnam] = nil

        mod = nil
        collectgarbage('collect')

        return 0
    end

    error(string.format('module %q not loaded', modnam), 2)
end

function modules.loaded()
    -- Weak-valued result table, so storing the result won't prevent the
    -- modules unloading.
    local mods = setmetatable({}, { __mode = 'v' })

    for name, mod in pairs(modules.__loaded) do
        table.insert(mods, {
            name = mod.name,
            description = mod.descr,
            context = mod.exports
        })
    end

    return mods
end

----
-- Root event handler
----
function __handle_event(event, ...)
    --logger:derive('__handle_event'):log('info', idumper(...))
    signal.dispatch(event, ...)
end


local function __handle_command(rsptgt, command, arg)
    print(command)
    if command == 'ping' then
        rsptgt:respond('pong')
    end
end


-- Prevent signals from being GC'd prematurely. (see above)
bot.__core_signals = {
    public_command = signal.bind('public_command', function(a, b, c, d)
        local chan = irc.channel.new(b)
        __handle_command(chan:find_user(a), c, d)
    end),

    private_command = signal.bind('private_command', function(a, c, d)
        __handle_command(irc.source.new(a), c, d)
    end)
}
