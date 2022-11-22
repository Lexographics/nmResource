#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

struct FileRule;
// nmres --recursive --cwd ./ --namespace Res --suffix .h

// defaults
std::string default_namespace_name = "Res";
std::string default_resource_suffix = ".h";
std::string default_rules_file = "./res_rules.txt";

// flags
bool flags_recursive = false;

// runtime data
std::vector<FileRule> res_rules;
std::filesystem::path res_cwd{"./"};

struct FileRule
{
    std::string suffix{""};
    std::string prefix{""};

    static bool Suitable(std::filesystem::path path, const FileRule& rule)
    {
        std::string pathStr = path.string();
        if(rule.prefix == "" && rule.suffix == "")
            return false;

        bool notsuitable = false;
        if(rule.prefix != "")
            if(!(pathStr.size() >= rule.prefix.size() && pathStr.compare(0, rule.prefix.size(), rule.prefix) == 0))
                { notsuitable = true; }
        if(rule.suffix != "")
            if(!(pathStr.size() >= rule.suffix.size() && pathStr.compare(pathStr.size()-rule.suffix.size(), rule.suffix.size(), rule.suffix) == 0))
                { notsuitable = true; }
        

        return !notsuitable;
    }

    FileRule(const std::string& rule)
    {
        if(rule.size() == 0) return;
        if(rule[0] == '#') return; // comments

        if(rule[0] == '>')
        {
            size_t spacePos = rule.find("*");
            if(spacePos == std::string::npos)
            {
                std::cout << "[nmres] File rule " << rule << " is invalid" << std::endl;
                // prefix and suffix left blank
            }
            else
            {
                prefix = (res_cwd / rule.substr(1, spacePos-1)).string();
                suffix = rule.substr(spacePos+1, rule.size()-1);

                #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__MINGW32__)
                std::replace(prefix.begin(), prefix.end(), '/', '\\');
                std::replace(suffix.begin(), suffix.end(), '/', '\\');
                #endif
            }
        }
        else
        {
            suffix = rule;
        }

        std::cout << "[nmres] Include file rule -> (prefix:'" << prefix << "', suffix: '" << suffix << "')" << std::endl;
    }
    ~FileRule() {}
};


bool EmbedFile(std::filesystem::path path);
bool IsAcceptedExtension(std::filesystem::path path);

int main(int argc, char const *argv[])
{
    std::vector<std::string> args;
    for(size_t i=0; i<argc; i++)
        args.push_back(argv[i]);
    
    for(size_t i =0; i<args.size(); i++)
    {
        std::string& arg = args[i];
        std::string nextarg = "";

        if(i < args.size() - 1)
        {
            nextarg = args[i+1];
        }

        if(arg == "-r")
            flags_recursive = true;

        else if(arg == "--recursive")
            flags_recursive = true;
        
        else if(arg == "--cwd")
            if(nextarg == "")
            {
                std::cerr << "No cwd specified. usage: --cwd 'dir/'" << std::endl;
                exit(1);
            }
            else
            {
                res_cwd = nextarg;
                i++; // skip nextarg
                continue;
            }
        else if(arg == "--namespace")
            if(nextarg == "")
            {
                std::cerr << "No namespace specified. usage: --namespace 'name'" << std::endl;
                exit(1);
            }
            else
            {
                default_namespace_name = nextarg;
                i++; // skip nextarg
                continue;
            }
        else if(arg == "--suffix")
            if(nextarg == "")
            {
                std::cerr << "No suffix specified. usage: --suffix '.ext'" << std::endl;
                exit(1);
            }
            else
            {
                default_resource_suffix = nextarg;
                i++; // skip nextarg
                continue;
            }
        else if(arg == "--rules") {
            if(nextarg == "")
            {
                std::cerr << "No rules file specified. usage: --rules 'file.txt'" << std::endl;
                exit(1);
            }
            else
            {
                default_rules_file = nextarg;
                i++; // skip nextarg
                continue;
            }
        }
    }

    std::ifstream ifstream(default_rules_file);
    if(ifstream.is_open())
    {
        std::string line{""};
        while(std::getline(ifstream, line))
        {
            res_rules.push_back(FileRule(line));
        }
    }
    
    std::cout << "\n";
    if(flags_recursive)
    {
        for(const auto& entry : std::filesystem::recursive_directory_iterator(res_cwd))
        {
            if(!entry.is_directory())
                if(IsAcceptedExtension(entry.path()))
                    EmbedFile(entry.path());
        }
    }
    else
    {
        for(const auto& entry : std::filesystem::directory_iterator(res_cwd))
        {
            if(!entry.is_directory())
                if(IsAcceptedExtension(entry.path()))
                    EmbedFile(entry.path());
        }
    }
    return 0;
}


bool EmbedFile(std::filesystem::path path)
{
    FILE* infile = fopen(path.string().c_str(), "rb");
    if(infile == nullptr)
    {
        std::cerr << "[nmres] Embed error: can't open source resource file " << path << std::endl;
        return false;
    }
    fseek(infile, 0, SEEK_END);
    unsigned int fileSize = ftell(infile);
    fseek(infile, 0, SEEK_SET);
    
    if(fileSize > 0)
    {
        char* buffer = new char[fileSize];
        size_t _read = fread(buffer, fileSize, 1, infile);
        fclose(infile);

        std::string arrayname = path.string();
        arrayname = std::filesystem::relative(path, res_cwd).string();
        std::replace(arrayname.begin(), arrayname.end(), '.', '_');
        std::replace(arrayname.begin(), arrayname.end(), '-', '_');
        std::replace(arrayname.begin(), arrayname.end(), '/', '_');
        std::replace(arrayname.begin(), arrayname.end(), '\\', '_');
        
        std::filesystem::path headerpath = path.concat(default_resource_suffix);
        std::ofstream ofstream(headerpath);
        if(!ofstream.is_open())
        {
            std::cerr << "[nmres] Embed error: can't open target resource file " << path << std::endl;
            delete[] buffer;
            return false;
        }

        ofstream << "#ifndef NMRESOURCE_EMBEDDED_RESOURCE_" << arrayname << "_H\n";
        ofstream << "#define NMRESOURCE_EMBEDDED_RESOURCE_" << arrayname << "_H\n";
        ofstream << "#pragma once\n\n";
        ofstream << "#include <array>\n";
        ofstream << "namespace " << default_namespace_name << "{\n";
        ofstream << "inline std::array<unsigned char, " << fileSize << "> " << arrayname << "_data\n";
        ofstream << "{\n";
        for(size_t i = 0; i<fileSize; i++)
        {
            if(i % 12 == 0)
                ofstream << "\n\t";
            ofstream << "0x" << std::hex << std::setw(2) << std::setfill('0') << (buffer[i] & 0xff) << ",";
        }
        ofstream << "\n};\n"; // array
        ofstream << "}\n"; // namespace
        ofstream << "#endif\n" << std::flush;

        std::cout << "[nmres] Embedded " << path << std::endl;
        delete[] buffer;
    }
    else
    {
        std::cerr << "[nmres] Embed error: can't open source resource file " << path << std::endl;
        return false;
    }

    return false;
}

bool IsAcceptedExtension(std::filesystem::path path)
{
    std::string pathStr = path.string();

    for(const auto& rule : res_rules)
        if(FileRule::Suitable(path, rule))
            return true;
    return false;
}
