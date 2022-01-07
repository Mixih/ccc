#include "argparse.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace ArgParse {

#define UNUSED(x) ((void)(x))

#define POSARG_GROUP_IDX 0
#define OPTARG_GROUP_IDX 1

//////////////////////////////////////////////
// ArgumentParser Implementations
//////////////////////////////////////////////
ArgumentParser::ArgumentParser() : termW{90}, progSet{false}, helpEn{true} {
    ArgGroup posargGroup(this, 0, "Positional Arguments", "", false);
    ArgGroup optargGroup(this, 1, "Options", "", false);
    groups.emplace_back(std::move(posargGroup));
    groups.emplace_back(std::move(optargGroup));
}

Args ArgumentParser::parseArgs(int argc, char** argv) {
    // add --help option if - is in the list of prefix characters or if no prefix
    // characters are set and the defualt help is enabled
    if (helpEn) {
        string::size_type found = pfxChars.find('-');
        if (!pfxChars.empty() && found != string::npos) {
            addArgument("-h", "--help")
                .action<HelpAction>()
                .help("show this help message and exit");
        } else {
            addArgument(pfxChars.substr(0, 0) + "h",
                        pfxChars.substr(0, 0) + pfxChars.substr(0, 0) + "help")
                .action<HelpAction>()
                .help("show this help message and exit");
        }
    }

    Args arguments;
    // insert default argument values
    for (const Action::UPtr& action : actions) {
        if (action->defaultval.has_value()) {
            arguments.args[action->dest] = action->defaultval;
        }
    }

    int cArg = 1;
    size_t cPosArg = 0;
    // parse arguments
    while (true) {
        string argStr = argv[cArg];
        Action* action = nullptr;
        OptKind optKind = getOptKind(argStr);

        vector<any> values;
        string err;
        if (optKind == OptKind::SHORT) {
            for (size_t i = 0; i < argStr.length() - 1; ++i) {
                auto it = optArgs.find(argStr.substr(i + 1, 1));
                if (it == optArgs.end()) {
                    // error: invalid short opt
                    cerr << "Argument Parsing Error: Invalid optional argument '"
                         << argStr << "'." << endl;
                }
                action = it->second;
                if (action->nargs != 0) {
                    // handle error as multiple short args must all take 0 args except for
                    // the last one
                    cerr << "Argument Parsing Error: optional argument '" << argStr[i]
                         << "' takes one or more parameters but zero given." << endl;
                }
                action->process(*this, arguments, {}, argStr.substr(i, 1), err);
            }
        } else if (optKind == OptKind::LONG) {
            size_t pos = argStr.find('=');
            if (pos != string::npos) {
                argStr = argStr.substr(0, pos); // trim
            }
            auto it = optArgs.find(argStr);
            if (it == optArgs.end()) {
                // error: invalid long opt
                cerr << "Argument Parsing Error: Invalid optional argument '" << argStr
                     << "'." << endl;
                return {};
            }
            action = it->second;
            if (pos != string::npos) {
                if (action->nargs != 1) {
                    // error, must take exactly one arg
                    cerr << "Argument Parsing Error: Assignment expression used for "
                            "argument '"
                         << argStr << "' that takes " << action->nargs << " parameters."
                         << endl;
                }
                values.emplace_back(convertType(
                    argStr.substr(pos + 1, argStr.size() - pos - 1), action->type));
            }
        } else if (optKind == OptKind::POS) {
            if (cPosArg >= posArgs.size()) {
                // throw error
                cerr << "Argument Parsing Error: Too many positional arguments specified."
                     << endl;
            }
            action = posArgs[cPosArg++];
        } else {
            // throw error
            cerr << "Internal error: invalid argument kind." << endl;
            return {};
        }

        for (long i = 0; i < action->nargs; ++i) {
            if (cArg > argc) {
                cerr << "Argument Parsing Error: Not enough arguments provided for "
                        "argument '"
                     << action->dest << "'." << endl;
            }
            values.emplace_back(convertType(argv[cArg++], action->type));
        }
        action->process(*this, arguments, values, argStr, err);
        if (cArg == argc) {
            break;
        }
    }

    // check all required args
    for (const Action::UPtr& action : actions) {
        if (action->required && !action->present) {
            // throw error
            cerr << "Argument Parsing Error: Required argument " << action->dest
                 << "not present." << endl;
        }
    }

    return arguments;
}

ArgumentParser::ArgBuilder ArgumentParser::addArgument(std::string nameOrFlags,
                                                       bool addToDefaultGroup) {
    Action::UPtr action = StoreAction::instantiate();
    ArgBuilder arg(this, action.get(), actions.size() - 1, addToDefaultGroup);
    arg.addNameOrFlag(move(nameOrFlags));
    actions.emplace_back(move(action));
    return arg;
}

ArgumentParser::ArgGroup ArgumentParser::addArgumentGroup(std::string name) {
    ArgGroup group = ArgGroup(this, groups.size(), move(name));
    groups.emplace_back(move(group));
    return *(groups.end() - 1);
}

ArgumentParser& ArgumentParser::prog(std::string name) {
    progName = move(name);
    return *this;
}

ArgumentParser& ArgumentParser::usage(std::string usageDesc) {
    usageText = move(usageDesc);
    return *this;
}

ArgumentParser& ArgumentParser::description(std::string desc) {
    descText = move(desc);
    return *this;
}

ArgumentParser& ArgumentParser::epilog(std::string epilogStr) {
    epilogText = move(epilogStr);
    return *this;
}

ArgumentParser& ArgumentParser::prefixChars(string pfxChars) {
    this->pfxChars = move(pfxChars);
    return *this;
}

ArgumentParser& ArgumentParser::addHelp(bool status) {
    helpEn = status;
    return *this;
}

ArgumentParser::OptKind ArgumentParser::getOptKind(const string& arg) {
    if (!arg.empty() && pfxChars.find(arg[0]) != string::npos) {
        if (arg.size() > 1 && arg[1] == arg[0]) {
            return OptKind::LONG;
        } else {
            return OptKind::SHORT;
        }
    }
    return OptKind::POS;
}

std::any ArgumentParser::convertType(const std::string& str, Type type) {
    switch (type) {
        case Type::CUSTOM:
        case Type::STRING:
            return str;
        case Type::INT:
            return stol(str);
        case Type::FLOAT:
            return stod(str);
    }
    return str;
}

void ArgumentParser::printHelp() {
    cout << "usage:" << usageText << endl;
    if (!descText.empty()) {
        cout << descText;
    }
    for (const ArgGroup& group : groups) {
        cout << group.name << endl;
        cout << endl;
        if (!group.desc.empty()) {
            cout << group.desc << endl;
            cout << endl;
        }
        for (const Action* action : group.actions) {
            unsigned long endIdx = action->nameFlags.size() - 1;
            for (size_t i = 0; i < endIdx; ++i) {
                cout << action->nameFlags[i] << ", ";
            }
            cout << action->nameFlags[endIdx];
            unsigned int padCol = group.maxAliasLen + 1;
            printPadded(action->helpText, padCol, termW - padCol, padCol);
            cout << endl;
        }
    }

    if (!epilogText.empty()) {
        cout << epilogText << endl;
    }
}

void ArgumentParser::printPadded(const std::string& str, long padTo, long wrapAt,
                                 long start) {
    stringstream ss(str);
    string buf;
    string pad(padTo, ' ');
    if (padTo > wrapAt) {
        return;
    }
    while (ss >> buf) {
        start += buf.length();
        if (start > wrapAt) {
            cout << endl;
            start -= wrapAt;
        }
        cout << buf;
    }
}

/////////////////////////////////////////////
// Action class implementations
////////////////////////////////////////////
Action::Action()
    : type(ArgumentParser::Type::STRING),
      groupIdx(0),
      nargs(0),
      required(false),
      present(false) {
}

template <typename T>
Optional<T> Action::getArgVal(Args& argsContainer, const string& argname) {
    auto it = argsContainer.args.find(argname);
    if (it == argsContainer.args.end()) {
        return {false, T()};
    }
    return {true, std::any_cast<T>(it->second)};
}

void Action::insertArg(Args& argsContainer, const std::string& dest, std::any val) {
    argsContainer.args[dest] = std::move(val);
}

bool Action::hasConflict(Args& parser, const string& arg) const {
    UNUSED(parser);
    UNUSED(arg);
    return present;
}

void Action::printHelp(ArgumentParser& parser) {
    parser.printHelp();
}

//////////////////////////////////////////////
// ArgBuilder implementations
//////////////////////////////////////////////
ArgumentParser::ArgBuilder::ArgBuilder(ArgumentParser* parser, Action* action,
                                       std::size_t ptrIdx, bool addToDefaultGroup)
    : parser{parser},
      actionRef{action},
      ptrIdx{ptrIdx},
      kind{ArgKind::NONE},
      addToDefaultGroup{addToDefaultGroup},
      destAdded{false} {
}

void ArgumentParser::ArgBuilder::addNameOrFlag(std::string str) {
    string::size_type found = parser->pfxChars.find(str[0]);
    if (found != string::npos) {
        // treat as an optional argument, not a positional argument
        if (kind == ArgKind::POS)
            throw runtime_error(
                "Can't specify positional argument alias for optional argument");
        if (kind == ArgKind::NONE && addToDefaultGroup) {
            parser->groups[OPTARG_GROUP_IDX].actions.push_back(actionRef);
            actionRef->groupIdx = OPTARG_GROUP_IDX;
            actionRef->nargs = 0;
            kind = ArgKind::OPT;
        }
        parser->optArgs[str] = actionRef;
    } else {
        // positional arguments are ordered by order of addition and thus just thrown into
        // the vector
        if (kind == ArgKind::OPT)
            throw runtime_error(
                "Can't specify optional argument alias for positional argument");
        if (kind == ArgKind::NONE && addToDefaultGroup) {
            parser->groups[POSARG_GROUP_IDX].actions.push_back(actionRef);
            actionRef->groupIdx = POSARG_GROUP_IDX;
            actionRef->nargs = 0;
            kind = ArgKind::POS;
        }
        parser->posArgs.push_back(actionRef);
    }
    // set destination to most recently added option
    OptKind optKind = parser->getOptKind(str);
    if (optKind == OptKind::SHORT && !destAdded) {
        actionRef->dest = str.substr(1);
    } else if (optKind == OptKind::LONG) {
        destAdded = true;
        actionRef->dest = str.substr(2);
    }
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::constVal(std::any value) {
    actionRef->constVal = std::move(value);
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::defaultVal(std::any value) {
    actionRef->defaultval = std::move(value);
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::dest(std::string destName) {
    actionRef->dest = std::move(destName);
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::help(std::string helptext) {
    actionRef->helpText = std::move(helptext);
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::metavar(std::string name) {
    actionRef->metavar = std::move(name);
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::nargs(long cnt) {
    actionRef->nargs = cnt;
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::required(bool val) {
    actionRef->required = val;
    return *this;
}

ArgumentParser::ArgBuilder& ArgumentParser::ArgBuilder::type(ArgumentParser::Type type) {
    actionRef->type = type;
    return *this;
}

//////////////////////////////////////////////
// ArgumentParser::ArgGroup implementations
//////////////////////////////////////////////
ArgumentParser::ArgGroup::ArgGroup(ArgumentParser* parser, size_t groupIdx,
                                   std::string name)
    : parser(parser),
      groupIdx(groupIdx),
      name(std::move(name)),
      maxAliasLen{0},
      mutex(false) {
}

ArgumentParser::ArgGroup::ArgGroup(ArgumentParser* parser, size_t groupIdx,
                                   std::string name, std::string desc, bool mutex)
    : parser(parser),
      groupIdx(groupIdx),
      name(std::move(name)),
      desc(std::move(desc)),
      maxAliasLen{0},
      mutex(mutex) {
}

ArgumentParser::ArgGroup& ArgumentParser::ArgGroup::setMutex(bool val) {
    mutex = val;
    return *this;
}

//////////////////////////////////////////////
// StoreAction implementation
//////////////////////////////////////////////
Action::UPtr StoreAction::instantiate() {
    return make_unique<StoreAction>();
}

bool StoreAction::process(ArgumentParser& parser, Args& args,
                          std::vector<std::any> values, std::string optStr,
                          string& error) {
    UNUSED(parser);

    if (hasConflict(args, this->dest)) {
        error = "Argument '" + optStr + "' is already defined.";
        return false;
    }

    if (values.size() > 1) {
        insertArg(args, this->dest, move(values));
    } else if (values.size() == 1) {
        insertArg(args, this->dest, move(values[0]));
    } else {
        insertArg(args, this->dest, this->defaultval);
    }
    present = true;
    return true;
}

//////////////////////////////////////////////
// StoreConstAction implementation
//////////////////////////////////////////////
Action::UPtr StoreConstAction::instantiate() {
    return make_unique<StoreConstAction>();
}

bool StoreConstAction::process(ArgumentParser& parser, Args& args,
                               std::vector<std::any> values, std::string optStr,
                               string& error) {
    UNUSED(parser);
    UNUSED(values);

    if (hasConflict(args, this->dest)) {
        error = "Argument '" + optStr + "' is already defined.";
        return false;
    }
    insertArg(args, this->dest, constVal);
    present = true;
    return true;
}

//////////////////////////////////////////////
// StoreTrueAction implementation
//////////////////////////////////////////////
Action::UPtr StoreTrueAction::instantiate() {
    return make_unique<StoreTrueAction>();
}

bool StoreTrueAction::process(ArgumentParser& parser, Args& args,
                              std::vector<std::any> values, std::string optStr,
                              string& error) {
    UNUSED(parser);
    UNUSED(values);

    if (hasConflict(args, this->dest)) {
        error = "Argument '" + optStr + "' is already defined.";
        return false;
    }
    insertArg(args, this->dest, true);
    return true;
}

//////////////////////////////////////////////
// StoreFalseAction implementation
//////////////////////////////////////////////
Action::UPtr StoreFalseAction::instantiate() {
    return make_unique<StoreFalseAction>();
}

bool StoreFalseAction::process(ArgumentParser& parser, Args& args,
                               std::vector<std::any> values, std::string optStr,
                               string& error) {
    UNUSED(parser);
    UNUSED(values);

    if (hasConflict(args, this->dest)) {
        error = "Argument '" + optStr + "' is already defined.";
        return false;
    }
    insertArg(args, this->dest, true);
    return true;
}

//////////////////////////////////////////////
// AppendAction implementation
//////////////////////////////////////////////
Action::UPtr AppendAction::instantiate() {
    return make_unique<AppendAction>();
}

bool AppendAction::process(ArgumentParser& parser, Args& args,
                           std::vector<std::any> values, std::string optStr,
                           string& error) {
    // TODO implement
    UNUSED(parser);
    UNUSED(args);
    UNUSED(values);
    UNUSED(optStr);
    UNUSED(error);

    return false;
}

//////////////////////////////////////////////
// AppendConstAction implementation
//////////////////////////////////////////////
Action::UPtr AppendConstAction::instantiate() {
    return make_unique<AppendConstAction>();
}

bool AppendConstAction::process(ArgumentParser& parser, Args& args,
                                std::vector<std::any> values, std::string optStr,
                                string& error) {
    // TODO implement
    UNUSED(parser);
    UNUSED(args);
    UNUSED(values);
    UNUSED(optStr);
    UNUSED(error);

    return false;
}

//////////////////////////////////////////////
// CountAction implementation
//////////////////////////////////////////////
Action::UPtr CountAction::instantiate() {
    return make_unique<CountAction>();
}

bool CountAction::process(ArgumentParser& parser, Args& args,
                          std::vector<std::any> values, std::string optStr,
                          string& error) {
    UNUSED(parser);
    UNUSED(values);
    UNUSED(optStr);
    UNUSED(error);

    long count = 0;
    if (present) {
        Optional<long> val = getArgVal<long>(args, optStr);
        if (!val.present) {
            error = "Internal error!";
            return false;
        }
        count = val.value;
    }
    insertArg(args, this->dest, count + 1);
    return true;
}

//////////////////////////////////////////////
// HelpAction implementation
//////////////////////////////////////////////
Action::UPtr HelpAction::instantiate() {
    return make_unique<HelpAction>();
}

bool HelpAction::process(ArgumentParser& parser, Args& args, std::vector<std::any> values,
                         std::string optStr, string& error) {
    UNUSED(args);
    UNUSED(values);
    UNUSED(optStr);
    UNUSED(error);

    printHelp(parser);
    return false;
}

} // namespace ArgParse