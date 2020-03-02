/*******************************************************************************
 * CLI - A simple command line interface.
 * Copyright (C) 2016 Daniele Pallastrelli
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef CLI_H_
#define CLI_H_

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cctype> // std::isspace
#include <type_traits>
#include <boost/lexical_cast.hpp>
#include "colorprofile.h"
#include "history.h"
#include "split.h"

#define CLI_DEPRECATED_API

#include <fstream> // @@@

namespace cli
{

    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    class HistoryStorage
    {
        public:
            virtual ~HistoryStorage() = default;
            virtual void Store(const std::vector<std::string>& commands) = 0;
            virtual std::vector<std::string> Commands() const = 0;
    };

    class LocalHistoryStorage : public HistoryStorage
    {
        public:
            explicit LocalHistoryStorage(std::size_t size = 1000) : maxSize(size) {}
            void Store(const std::vector<std::string>& cmds) override
            {
                commands.insert(commands.end(), cmds.begin(), cmds.end());
                if (commands.size() > maxSize)
                    commands.erase(commands.begin(), commands.begin()+commands.size()-maxSize);
            }
            std::vector<std::string> Commands() const override
            {
                return std::vector<std::string>(commands.begin(), commands.end());
            }
        private:
            const std::size_t maxSize;
            std::deque<std::string> commands;
    };

    class FileHistoryStorage : public HistoryStorage
    {
        public:
            explicit FileHistoryStorage(const std::string& _fileName = ".cli") :
                fileName(_fileName)
            {}
            void Store(const std::vector<std::string>& cmds) override
            {
                std::ofstream f(fileName, std::ios_base::app | std::ios_base::out);
                for (const auto& line: cmds)
                    f << line << '\n';
            }
            std::vector<std::string> Commands() const override
            {
                std::vector<std::string> commands;
                std::ifstream in(fileName);
                if (in)
                {
                    std::string line;
                    while (std::getline(in, line))
                        commands.push_back(line);
                }
                return commands;
            }
        private:
            const std::string fileName;
    };

    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


    // ********************************************************************

    template < typename T > struct TypeDesc {};
    template <> struct TypeDesc< char > { static const char* Name() { return "<char>"; } };
    template <> struct TypeDesc< unsigned char > { static const char* Name() { return "<unsigned char>"; } };
    template <> struct TypeDesc< short > { static const char* Name() { return "<short>"; } };
    template <> struct TypeDesc< unsigned short > { static const char* Name() { return "<unsigned short>"; } };
    template <> struct TypeDesc< int > { static const char* Name() { return "<int>"; } };
    template <> struct TypeDesc< unsigned int > { static const char* Name() { return "<unsigned int>"; } };
    template <> struct TypeDesc< long > { static const char* Name() { return "<long>"; } };
    template <> struct TypeDesc< unsigned long > { static const char* Name() { return "<unsigned long>"; } };
    template <> struct TypeDesc< float > { static const char* Name() { return "<float>"; } };
    template <> struct TypeDesc< double > { static const char* Name() { return "<double>"; } };
    template <> struct TypeDesc< long double > { static const char* Name() { return "<long double>"; } };
    template <> struct TypeDesc< bool > { static const char* Name() { return "<bool>"; } };
    template <> struct TypeDesc< std::string > { static const char* Name() { return "<string>"; } };

    // ********************************************************************

    // forward declarations
    class Menu;
    class CliSession;


    class Cli
    {

        // inner class to provide a global output stream
        class OutStream
        {
        public:
            template <typename T>
            OutStream& operator << (const T& msg)
            {
                for (auto out: ostreams)
                    *out << msg;
                return *this;
            }

            // this is the type of std::cout
            typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
            // this is the function signature of std::endl
            typedef CoutType& (*StandardEndLine)(CoutType&);

            // takes << std::endl
            OutStream& operator << (StandardEndLine manip)
            {
                for (auto out: ostreams)
                    manip(*out);
                return *this;
            }

        private:
            friend class Cli;

            void Register(std::ostream& o)
            {
                ostreams.push_back(&o);
            }
            void UnRegister(std::ostream& o)
            {
                ostreams.erase(std::remove(ostreams.begin(), ostreams.end(), &o), ostreams.end());
            }

            std::vector<std::ostream*> ostreams;
        };
        // end inner class

    public:
        Cli(
            std::unique_ptr<Menu>&& _rootMenu,
            std::function< void(std::ostream&)> _exitAction = std::function< void(std::ostream&) >(),
            std::unique_ptr<HistoryStorage> historyStorage = std::make_unique<LocalHistoryStorage>()
        ) :
            globalHistoryStorage(std::move(historyStorage)),
            rootMenu(std::move(_rootMenu)),
            exitAction(_exitAction)
        {
        }

        // disable value semantics
        Cli(const Cli&) = delete;
        Cli& operator = (const Cli&) = delete;

        Menu* RootMenu() { return rootMenu.get(); }
        void ExitAction( std::function< void(std::ostream&)> action ) { exitAction = action; }
        void ExitAction( std::ostream& out ) { if ( exitAction ) exitAction( out ); }

        static void Register(std::ostream& o) { cout().Register(o); }
        static void UnRegister(std::ostream& o) { cout().UnRegister(o); }

        static OutStream& cout()
        {
            static OutStream s;
            return s;
        }

        void StoreCommands(const std::vector<std::string>& cmds)
        {
            globalHistoryStorage->Store(cmds);
        }

        std::vector<std::string> GetCommands() const
        {
            return globalHistoryStorage->Commands();
        }

    private:
        std::unique_ptr<HistoryStorage> globalHistoryStorage;
        std::unique_ptr<Menu> rootMenu; // just to keep it alive
        std::function<void(std::ostream&)> exitAction;
    };

    // ********************************************************************

    class Command
    {
    public:
        explicit Command(const std::string& _name) : name(_name), enabled(true) {}
        virtual ~Command() = default;
        virtual void Enable() { enabled = true; }
        virtual void Disable() { enabled = false; }
        virtual bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) = 0;
        virtual void Help( std::ostream& out ) const = 0;
        // Returns the collection of completions relatives to this command.
        // For simple commands, provides a base implementation that use the name of the command
        // for aggregate commands (i.e., Menu), the function is redefined to give the menu command
        // and the subcommand recursively
        virtual std::vector<std::string> GetCompletionRecursive(const std::string& line) const
        {
            if (!enabled) return {};
            if (name.rfind(line, 0) == 0) return {name}; // name starts_with line
            else return {};
        }
    protected:
        const std::string& Name() const { return name; }
        bool IsEnabled() const { return enabled; }
    private:
        const std::string name;
        bool enabled;
    };

    // ********************************************************************

    // free utility function to get completions from a list of commands and the current line
    inline std::vector<std::string> GetCompletions(
        const std::shared_ptr<std::vector<std::shared_ptr<Command>>>& cmds, 
        const std::string& currentLine)
    {
        std::vector<std::string> result;
        std::for_each(cmds->begin(), cmds->end(),
            [&currentLine,&result](const auto& cmd)
            {
                auto c = cmd->GetCompletionRecursive(currentLine);
                result.insert(
                    result.end(),
                    std::make_move_iterator(c.begin()),
                    std::make_move_iterator(c.end())
                );
            }
        );
        return result;
    }

    // ********************************************************************

    class CliSession
    {
    public:
        CliSession(Cli& _cli, std::ostream& _out, std::size_t historySize = 100);
        ~CliSession() { cli.UnRegister(out); }

        // disable value semantics
        CliSession(const CliSession&) = delete;
        CliSession& operator = (const CliSession&) = delete;

        void Feed( const std::string& cmd );

        void Prompt();

        void Current(Menu* menu) { current = menu; }

        std::ostream& OutStream() { return out; }

        void Help() const;

        void Exit()
        {
            if (exitAction) exitAction(out);
            cli.ExitAction(out);

            auto cmds = history.GetCommands();
            cli.StoreCommands(cmds);
        }

        void ExitAction(const std::function<void(std::ostream&)>& action)
        {
            exitAction = action;
        }

        void ShowHistory() const { history.Show(out); }

        std::string PreviousCmd(const std::string& line)
        {
            return history.Previous(line);
        }

        std::string NextCmd()
        {
            return history.Next();
        }

        std::vector<std::string> GetCompletions(std::string currentLine) const;

    private:

        Cli& cli;
        Menu* current;
        std::unique_ptr<Menu> globalScopeMenu;
        std::ostream& out;
        std::function< void(std::ostream&)> exitAction;
        detail::History history;
    };

    // ********************************************************************

    class CmdHandler
    {
    public:
        using CmdVec = std::vector<std::shared_ptr<Command>>;
        CmdHandler() : descriptor(std::make_shared<Descriptor>()) {}
        CmdHandler(std::weak_ptr<Command> c, std::weak_ptr<CmdVec> v) :
            descriptor(std::make_shared<Descriptor>(c, v))
        {}
        void Enable() { if (descriptor) descriptor->Enable(); }
        void Disable() { if (descriptor) descriptor->Disable(); }
        void Remove() { if (descriptor) descriptor->Remove(); }
    private:
        struct Descriptor
        {
            Descriptor() {}
            Descriptor(const std::weak_ptr<Command>& c, const std::weak_ptr<CmdVec>& v) :
                cmd(c), cmds(v)
            {}
            void Enable()
            { 
                if (auto c = cmd.lock())
                    c->Enable();
            }
            void Disable()
            {
                if(auto c = cmd.lock())
                    c->Disable();
            }
            void Remove()
            {
                auto scmd = cmd.lock();
                auto scmds = cmds.lock();
                if (scmd && scmds)
                {
                    auto i = std::find_if(
                        scmds->begin(),
                        scmds->end(),
                        [&](const auto& c){ return c.get() == scmd.get(); }
                    );
                    if (i != scmds->end())
                        scmds->erase(i);
                }
            }
            std::weak_ptr<Command> cmd;
            std::weak_ptr<CmdVec> cmds;
        };
        std::shared_ptr<Descriptor> descriptor;
    };

    // ********************************************************************

    class Menu : public Command
    {
    public:
        // disable value semantics
        Menu(const Menu&) = delete;
        Menu& operator = (const Menu&) = delete;

        Menu() : Command({}), parent(nullptr), description(), cmds(std::make_shared<Cmds>()) {}

        Menu( const std::string& _name, const std::string& desc = "(menu)" ) :
            Command(_name), parent(nullptr), description(desc), cmds(std::make_shared<Cmds>())
        {}

        template <typename F>
        CmdHandler Insert(const std::string& name, F f, const std::string& help = "", const std::vector<std::string>& parDesc={})
        {
            // dispatch to private Insert methods
            return Insert(name, help, parDesc, f, &F::operator());
        }

        template <typename F>
        CmdHandler Insert(const std::string& name, const std::vector<std::string>& parDesc, F f, const std::string& help = "")
        {
            // dispatch to private Insert methods
            return Insert(name, help, parDesc, f, &F::operator());
        }

#ifdef CLI_DEPRECATED_API
        template <typename F>
        [[deprecated("Use the method Insert instead")]]
        void Add(const std::string& name, F f, const std::string& help = "")
        {
            // dispatch to private Add methods
            Add(name, help, f, &F::operator());
        }

        [[deprecated("Use the method Insert instead")]]
        void Add(std::unique_ptr<Command>&& cmd)
        {
            std::shared_ptr<Command> s(std::move(cmd));
            cmds->push_back(s);
        }

        [[deprecated("Use the method Insert instead")]]
        void Add(std::unique_ptr<Menu>&& menu)
        {
            std::shared_ptr<Menu> s(std::move(menu));
            s->parent = this;
            cmds->push_back(s);
        }
#endif // CLI_DEPRECATED_API
        
        CmdHandler Insert(std::unique_ptr<Command>&& cmd)
        {
            std::shared_ptr<Command> scmd(std::move(cmd));
            CmdHandler c(scmd, cmds);
            cmds->push_back(scmd);
            return c;
        }

        CmdHandler Insert(std::unique_ptr<Menu>&& menu)
        {
            std::shared_ptr<Menu> smenu(std::move(menu));
            CmdHandler c(smenu, cmds);
            smenu->parent = this;
            cmds->push_back(smenu);
            return c;
        }

        bool Exec(const std::vector<std::string>& cmdLine, CliSession& session) override
        {
            if (!IsEnabled()) return false;
            if ( cmdLine[ 0 ] == Name() )
            {
                if ( cmdLine.size() == 1 )
                {
                    session.Current( this );
                    return true;
                }
                else
                {
                    // check also for subcommands
                    std::vector<std::string > subCmdLine(cmdLine.begin()+1, cmdLine.end());
                    for (auto& cmd: *cmds)
                        if (cmd->Exec( subCmdLine, session )) return true;
                }
            }
            return false;
        }

        bool ScanCmds(const std::vector<std::string>& cmdLine, CliSession& session)
        {
            if (!IsEnabled()) return false;
            for (auto& cmd: *cmds)
                if (cmd->Exec(cmdLine, session)) return true;
            if (parent && parent->Exec(cmdLine, session)) return true;
            return false;
        }

        std::string Prompt() const
        {
            return Name();
        }

        void MainHelp(std::ostream& out)
        {
            if (!IsEnabled()) return;
            for (const auto& cmd: *cmds)
                cmd->Help(out);
            if (parent) parent->Help(out);
        }

        void Help(std::ostream& out) const override
        {
            if (!IsEnabled()) return;
            out << " - " << Name() << "\n\t" << description << "\n";
        }

        std::vector<std::string> GetCompletions(const std::string& currentLine) const
        {
            auto result = cli::GetCompletions(cmds, currentLine);
            if (parent)
            {
                auto c = parent->GetCompletionRecursive(currentLine);
                result.insert(result.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
            }
            return result;
        }

        virtual std::vector<std::string> GetCompletionRecursive(const std::string& line) const override
        {
            if (line.rfind(Name(), 0) == 0) // line starts_with Name()
            {
                auto rest = line;
                rest.erase(0, Name().size());
                // trim_left(rest);
                rest.erase(rest.begin(), std::find_if(rest.begin(), rest.end(), [](int ch) { return !std::isspace(ch); }));
                std::vector<std::string> result;
                for (const auto& cmd: *cmds)
                {
                    auto cs = cmd->GetCompletionRecursive(rest);
                    for (const auto& c: cs)
                        result.push_back(Name() + ' ' + c); // concat submenu with command
                }
                return result;
            }
            return Command::GetCompletionRecursive(line);
        }

    private:

#ifdef CLI_DEPRECATED_API
        template <typename F, typename R>
        void Add(const std::string& name, const std::string& help, F& f,R (F::*mf)(std::ostream& out) const);

        template <typename F, typename R, typename A1>
        void Add(const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, std::ostream& out) const);

        template <typename F, typename R, typename A1, typename A2>
        void Add(const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, std::ostream& out) const);

        template <typename F, typename R, typename A1, typename A2, typename A3>
        void Add(const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, A3, std::ostream& out) const);

        template <typename F, typename R, typename A1, typename A2, typename A3, typename A4>
        void Add(const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, A3, A4, std::ostream& out) const);
#endif // CLI_DEPRECATED_API

        template <typename F, typename R, typename ... Args>
        CmdHandler Insert(const std::string& name, const std::string& help, const std::vector<std::string>& parDesc, F& f, R (F::*)(std::ostream& out, Args...) const);

        Menu* parent;
        const std::string description;
        // using shared_ptr instead of unique_ptr to get a weak_ptr
        // for the CmdHandler::Descriptor
        using Cmds = std::vector<std::shared_ptr<Command>>;
        std::shared_ptr<Cmds> cmds;
    };

    // ********************************************************************

#ifdef CLI_DEPRECATED_API

    class FuncCmd : public Command
    {
    public:
        // disable value semantics
        FuncCmd( const FuncCmd& ) = delete;
        FuncCmd& operator = ( const FuncCmd& ) = delete;

        FuncCmd(
            const std::string& _name,
            std::function< void( std::ostream& )> _function,
            const std::string& desc = ""
        ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 1 ) return false;
            if ( cmdLine[ 0 ] == Name() )
            {
                function( session.OutStream() );
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name() << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( std::ostream& ) > function;
        const std::string description;
    };

    template < typename T >
    class FuncCmd1 : public Command
    {
    public:
        // disable value semantics
        FuncCmd1( const FuncCmd1& ) = delete;
        FuncCmd1& operator = ( const FuncCmd1& ) = delete;

        FuncCmd1(
            const std::string& _name,
            std::function< void( T, std::ostream& ) > _function,
            const std::string& desc = ""
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 2 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T arg = boost::lexical_cast<T>( cmdLine[ 1 ] );
                    function( arg, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2 >
    class FuncCmd2 : public Command
    {
    public:
        // disable value semantics
        FuncCmd2( const FuncCmd2& ) = delete;
        FuncCmd2& operator = ( const FuncCmd2& ) = delete;

        FuncCmd2(
            const std::string& _name,
            std::function< void( T1, T2, std::ostream& ) > _function,
            const std::string& desc = "2 parameter command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 3 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    function( arg1, arg2, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2, typename T3 >
    class FuncCmd3 : public Command
    {
    public:
        // disable value semantics
        FuncCmd3( const FuncCmd3& ) = delete;
        FuncCmd3& operator = ( const FuncCmd3& ) = delete;

        FuncCmd3(
            const std::string& _name,
            std::function< void( T1, T2, T3, std::ostream& ) > _function,
            const std::string& desc = "3 parameters command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 4 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    T3 arg3 = boost::lexical_cast<T3>( cmdLine[ 3 ] );
                    function( arg1, arg2, arg3, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << " " << TypeDesc< T3 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, T3, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2, typename T3, typename T4 >
    class FuncCmd4 : public Command
    {
    public:
        // disable value semantics
        FuncCmd4( const FuncCmd4& ) = delete;
        FuncCmd4& operator = ( const FuncCmd4& ) = delete;

        FuncCmd4(
            const std::string& _name,
            std::function< void( T1, T2, T3, T4, std::ostream& ) > _function,
            const std::string& desc = "4 parameters command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 5 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    T3 arg3 = boost::lexical_cast<T3>( cmdLine[ 3 ] );
                    T4 arg4 = boost::lexical_cast<T4>( cmdLine[ 4 ] );
                    function( arg1, arg2, arg3, arg4, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << " " << TypeDesc< T3 >::Name()
                << " " << TypeDesc< T4 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, T3, T4, std::ostream& )> function;
        const std::string description;
    };

#endif // CLI_DEPRECATED_API

    // *******************************************

    template <typename F, typename ... Args>
    struct Select;

    template <typename F, typename P, typename ... Args>
    struct Select<F, P, Args...>
    {
        template <typename InputIt>
        static void Exec(const F& f, InputIt first, InputIt last)
        {
            assert( first != last );
            assert( std::distance(first, last) == 1+sizeof...(Args) );
            const P p = boost::lexical_cast<typename std::decay<P>::type>(*first);
            auto g = [&](auto ... pars){ f(p, pars...); };
            Select<decltype(g), Args...>::Exec(g, std::next(first), last);
        }
    };

    template <typename F>
    struct Select<F>
    {
        template <typename InputIt>
        static void Exec(const F& f, InputIt first, InputIt last)
        {
            assert(first == last);
            f();
        }
    };

    template <typename ... Args>
    struct PrintDesc;

    template <typename P, typename ... Args>
    struct PrintDesc<P, Args...>
    {
        static void Dump(std::ostream& out)
        {
            out << " " << TypeDesc< typename std::decay<P>::type >::Name();
            PrintDesc<Args...>::Dump(out);
        }
    };

    template <>
    struct PrintDesc<>
    {
        static void Dump(std::ostream& /*out*/) {}
    };

    // *******************************************

    template <typename F, typename ... Args>
    class VariadicFunctionCommand : public Command
    {
    public:
        // disable value semantics
        VariadicFunctionCommand(const VariadicFunctionCommand&) = delete;
        VariadicFunctionCommand& operator = (const VariadicFunctionCommand&) = delete;

        VariadicFunctionCommand(
            const std::string& _name,
            F fun,
            const std::string& desc = "unknown command",
            const std::vector<std::string>& parDesc = {}
        )
            : Command(_name), func(std::move(fun)), description(desc), parameterDesc(parDesc)
        {
        }

        bool Exec(const std::vector< std::string >& cmdLine, CliSession& session) override
        {
            if (!IsEnabled()) return false;
            const std::size_t paramSize = sizeof...(Args);
            if (cmdLine.size() != paramSize+1) return false;
            if (Name() == cmdLine[0])
            {
                try
                {
                    auto g = [&](auto ... pars){ func( session.OutStream(), pars... ); };
                    Select<decltype(g), Args...>::Exec(g, std::next(cmdLine.begin()), cmdLine.end());
                }
                catch (boost::bad_lexical_cast &)
                {
                    return false;
                }
                return true;
            }
            return false;
        }
        void Help(std::ostream& out) const override
        {
            if (!IsEnabled()) return;
            out << " - " << Name();
            if (parameterDesc.empty())
                PrintDesc<Args...>::Dump(out);
            for (auto& s: parameterDesc)
                out << " <" << s << '>';
            out << "\n\t" << description << "\n";
        }

    private:

        const F func;
        const std::string description;
        const std::vector<std::string> parameterDesc;
    };


    // ********************************************************************

    // CliSession implementation

    inline CliSession::CliSession(Cli& _cli, std::ostream& _out, std::size_t historySize) :
            cli(_cli),
            current(cli.RootMenu()),
            globalScopeMenu(std::make_unique< Menu >()),
            out(_out),
            history(historySize)
        {
            history.LoadCommands(cli.GetCommands());

            cli.Register(out);
            globalScopeMenu->Insert(
                "help",
                [this](std::ostream&){ Help(); },
                "This help message"
            );
            globalScopeMenu->Insert(
                "exit",
                [this](std::ostream&){ Exit(); },
                "Quit the session"
            );
#ifdef CLI_HISTORY_CMD
            globalScopeMenu->Insert(
                "history",
                [this](std::ostream&){ ShowHistory(); },
                "Show the history"
            );
#endif
        }

    inline void CliSession::Feed(const std::string& cmd)
    {
        std::vector<std::string> strs;
        detail::split(strs, cmd);
        if (strs.empty()) return; // just hit enter

        history.NewCommand(cmd); // add anyway to history
        
        // global cmds check
        bool found = globalScopeMenu->ScanCmds(strs, *this);

        // root menu recursive cmds check
        if (!found) found = current -> ScanCmds(std::move(strs), *this); // last use of strs

        if (!found) // error msg if not found
            out << "Command unknown: " << cmd << "\n";

        return;
    }

    inline void CliSession::Prompt()
    {
        out << beforePrompt
            << current -> Prompt()
            << afterPrompt
            << "> "
            << std::flush;
    }

    inline void CliSession::Help() const
    {
        out << "Commands available:\n";
        globalScopeMenu->MainHelp(out);
        current -> MainHelp( out );
    }

    inline std::vector<std::string> CliSession::GetCompletions(std::string currentLine) const
    {
        // trim_left(currentLine);
        currentLine.erase(currentLine.begin(), std::find_if(currentLine.begin(), currentLine.end(), [](int ch) { return !std::isspace(ch); }));
        auto v1 = globalScopeMenu->GetCompletions(currentLine);
        auto v3 = current->GetCompletions(currentLine);
        v1.insert(v1.end(), std::make_move_iterator(v3.begin()), std::make_move_iterator(v3.end()));
        return v1;
    }

    // Menu implementation

#ifdef CLI_DEPRECATED_API
    template < typename F, typename R >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(std::ostream& out) const )
    {
        cmds->push_back(std::make_shared<FuncCmd>(name, f, help));
    }

    template < typename F, typename R, typename A1 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, std::ostream& out) const )
    {
        cmds->push_back(std::make_shared<FuncCmd1<A1>>(name, f, help));
    }

    template < typename F, typename R, typename A1, typename A2 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, std::ostream& out) const )
    {
        cmds->push_back(std::make_shared<FuncCmd2<A1, A2>>(name, f, help));
    }

    template < typename F, typename R, typename A1, typename A2, typename A3 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, A3, std::ostream& out) const )
    {
        cmds->push_back(std::make_shared<FuncCmd3<A1, A2, A3>>(name, f, help));
    }

    template < typename F, typename R, typename A1, typename A2, typename A3, typename A4 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, A3, A4, std::ostream& out) const )
    {
        cmds->push_back(std::make_shared<FuncCmd4<A1, A2, A3, A4>>(name, f, help));
    }
#endif // CLI_DEPRECATED_API

    template <typename F, typename R, typename ... Args>
    CmdHandler Menu::Insert(const std::string& name, const std::string& help, const std::vector<std::string>& parDesc, F& f, R (F::*)(std::ostream& out, Args...) const )
    {
        auto c = std::make_shared<VariadicFunctionCommand<F, Args ...>>(name, f, help, parDesc);
        CmdHandler cmd(c, cmds);
        cmds->push_back(c);
        return cmd;
    }

} // namespace

#endif
