/**
 * A CLI argument parser interface inspired (very heavily) by the Python argparse system.
 */
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <any>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ArgParse {

// forward declare classes needed by the ArgBuilder Parser
class Action;
class Args;

/**
 * A Parser for commandline arguments.
 *
 * Note that this type is a _move only_ type, and should always be returned by value. Copy
 * semantics are not supported at all for this type.
 */
class ArgumentParser {
public:
    /**
     * Types used for basic argument type conversion. More complex
     * conversions should be implemented as actions (e.g. setting type to bool is probably
     * incorrect, as it will flag as true for any user input that is not zero).
     */
    enum class Type
    {
        STRING,
        INT,
        FLOAT,
        CUSTOM
    };
    // forward declare group class
    class ArgGroup;

private:
    // class defs
    // forward declare argument class
    class ArgBuilder;
    friend class Action;
    enum class OptKind
    {
        SHORT,
        LONG,
        POS,
    };
    enum class ArgKind
    {
        NONE,
        OPT,
        POS
    };

private:
    // Class field section
    std::unordered_map<std::string, Action*> optArgs;
    std::vector<Action*> posArgs;
    std::vector<std::unique_ptr<Action>> actions;
    std::vector<ArgGroup> groups;

    std::string pfxChars;
    std::string progName;
    std::string usageText;
    std::string descText;
    std::string epilogText;
    const long termW;
    bool progSet;
    bool helpEn;

private:
    OptKind getOptKind(const std::string& arg);
    std::any convertType(const std::string& str, Type type);
    void printHelp();
    static void printPadded(const std::string& str, long padTo, long wrapAt,
                            long start = 0);

public:
    ArgumentParser();
    ~ArgumentParser() = default;
    // copy semantics are explicitly disabled
    ArgumentParser(const ArgumentParser& parser) = delete;
    ArgumentParser& operator=(const ArgumentParser& parser) noexcept = delete;
    // move semantics are explicitly enabled with default implementation
    ArgumentParser(ArgumentParser&& parser) = default;
    ArgumentParser& operator=(ArgumentParser&& parser) noexcept = default;

    Args parseArgs(int argc, char** argv);

    /**
     * Add an argument to be parsed by the argument parser. Arguments, that don't start
     * with dashes will be interpreted as positional arguments arguments that start with
     * '-' will be interpreted as flag arguments, and arguments that start with '--' will
     * be interpreted as "long arguments".
     *
     * This is the recursive case for the variadic function
     *
     * @tparam T must be std::string or compatible type.
     * @param nameOrFlags name or flag to use for argument.
     * @param args extra names or flags to associate with the same argument.
     * @return the ArgBuilder object, ready for call chaining to configure features.
     */
    template <typename... T> ArgBuilder addArgument(std::string nameOrFlags, T&&... args);
    /**
     * Add an argument to be parsed by the argument parser. Arguments, that don't start
     * with dashes will be interpreted as positional arguments arguments that start with
     * '-' will be interpreted as flag arguments, and arguments that start with '--' will
     * be interpreted as "long arguments".
     *
     * This is the base case for the variadic function
     *
     * @param nameOrFlags name or flag to use for argument.
     * @param addToDefaultGroup if true, adds parameters to the default position/optional
     * arguments groups.
     * @return the ArgBuilder object, ready for call chaining to configure features.
     */
    ArgBuilder addArgument(std::string nameOrFlags, bool addToDefaultGroup = true);

    ArgGroup addArgumentGroup(std::string name);

    // The following methods set attributes on the argument parser directly, and can be
    // call chained.
    ArgumentParser& prog(std::string name);
    ArgumentParser& usage(std::string usageDesc);
    ArgumentParser& description(std::string desc);
    ArgumentParser& epilog(std::string epilogStr);
    ArgumentParser& prefixChars(std::string pfxChars);
    ArgumentParser& addHelp(bool status);
};

/**
 * A class used for configuring command line arguments. Calls must be directly chained
 * onto the argument as no external instances may be created.
 */
class ArgumentParser::ArgBuilder {
private:
    ArgumentParser* parser;
    Action* actionRef;
    std::size_t ptrIdx;
    ArgKind kind;
    bool addToDefaultGroup;
    bool destAdded;

    ArgBuilder(ArgumentParser* parser, Action* action, std::size_t ptrIdx,
               bool addToDefaultGroup);

    friend class ArgumentParser;

protected:
    void addNameOrFlag(std::string str);

public:
    template <typename T> ArgBuilder& action();
    template <typename T> ArgBuilder& choices(std::vector<T> choices);
    ArgBuilder& constVal(std::any value);
    ArgBuilder& defaultVal(std::any value);
    ArgBuilder& dest(std::string destName);
    ArgBuilder& help(std::string helptext);
    ArgBuilder& metavar(std::string name);
    ArgBuilder& nargs(long cnt);
    ArgBuilder& required(bool val);
    ArgBuilder& type(Type type);
};

template <typename T> struct Optional {
    bool present;
    T value;
};

/**
 * A group of arguments used to group args together in help and apply options to multiple
 * arguments at once.
 */
class ArgumentParser::ArgGroup {
private:
    ArgumentParser* parser;
    std::size_t groupIdx;
    std::string name;
    std::string desc;
    std::vector<Action*> actions;
    int maxAliasLen;
    bool mutex;

    ArgGroup(ArgumentParser* parser, size_t groupIdx, std::string name);
    ArgGroup(ArgumentParser* parser, size_t groupIdx, std::string name, std::string desc,
             bool mutex);

    friend class ArgumentParser;

public:
    ArgGroup& setMutex(bool val);
    template <typename... T> ArgBuilder addArgument(std::string nameOrFlags, T&&... args);
};

/**
 * Parsed arguments ready for use.
 */
class Args {
private:
    std::unordered_map<std::string, std::any> args;

    friend class Action;
    friend class ArgumentParser;

public:
    template <typename T> struct Entry;

    template <typename T> Entry<T> get(const std::string& argRef);
};

/**
 * An entry from an args object containing whether or not it was found, and the value if
 * it was found.
 *
 * @tparam T type of entry.
 */
template <typename T> struct Args::Entry {
    bool present;
    T val;
};

/**
 * An action used to determine how an argument will be stored and processed.
 */
class Action {
private:
    friend class ArgumentParser;

protected:
    std::string helpText;
    std::any constVal;
    std::any defaultval;
    std::string dest;
    std::string metavar;
    std::vector<std::any> choices;
    std::vector<std::string> nameFlags;
    ArgumentParser::Type type;
    std::size_t groupIdx;
    long nargs;
    bool required;
    bool present;

protected:
    Action();

    template <typename T>
    Optional<T> getArgVal(Args& argsContainer, const std::string& argname);
    static void insertArg(Args& argsContainer, const std::string& dest, std::any val);
    bool hasConflict(Args& parser, const std::string& arg) const;
    static void printHelp(ArgumentParser& parser);

public:
    typedef std::unique_ptr<Action> UPtr;

    /**
     * All implementing classes must override this method to do something with
     * the argument.
     *
     * @param parser Argument parser action is attached to.
     * @param args Arguments object to add parsed arg to.
     * @param values Values obtained from the argument values.
     * @param error An error string to return to the parser if the action fails.
     * @return
     */
    virtual bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                         std::string optStr, std::string& error) = 0;
};

/////////////////////////////////////////////////////////////
// Default action class declarations
/////////////////////////////////////////////////////////////

class StoreAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class StoreConstAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class StoreTrueAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class StoreFalseAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class AppendAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class AppendConstAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class CountAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

class HelpAction : public Action {
public:
    static Action::UPtr instantiate();
    bool process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                 std::string optStr, std::string& error) override;
};

////////////////////////////////////
// template function implementations
////////////////////////////////////
template <typename T> Args::Entry<T> Args::get(const std::string& argRef) {
    auto it = args.find(argRef);
    if (it != args.end()) {
        try {
            return {true, std::any_cast<T>(it->second)};
        } catch (const std::bad_any_cast& e) {
            return {false, T()};
        }
    }
    return {false, T()};
}

template <typename T>
ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::choices(std::vector<T> choices) {
    actionRef->choices.clear();
    actionRef->choices.resize(actionRef->choices.size());
    for (T& item : choices) {
        actionRef->choices.push_back(item);
    }
    return *this;
}
template <typename T> ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::action() {
    static_assert(std::is_convertible_v<T*, Action*>,
                  "Error: All actions must derive from ArgumentParser::Action.");
    Action::UPtr action = T::instantiate();

    if (actionRef) {
        parser->actions.erase(parser->actions.begin() + ptrIdx);
    }
    actionRef = action.get();
    parser->actions.emplace_back(std::move(action));
    return *this;
}

template <typename... T>
ArgumentParser::ArgBuilder ArgumentParser::addArgument(std::string nameOrFlags,
                                                       T&&... args) {
    ArgBuilder argument = addArgument(std::forward<T>(args)...);
    argument.addNameOrFlag(std::move(nameOrFlags));
    return argument;
}

template <typename... T>
ArgumentParser::ArgBuilder ArgumentParser::ArgGroup::addArgument(std::string nameOrFlags,
                                                                 T&&... args) {
    ArgBuilder builder =
        parser->addArgument(nameOrFlags, std::forward<T>(args)..., false);
    parser->groups[groupIdx].actions.push_back(builder.actionRef);
    builder.actionRef->groupIdx = groupIdx;
    return builder;
}

} // namespace ArgParse
#endif // ARGPARSE_H