#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

// nmres --recursive --cwd ./ --namespace Res --suffix .h

// defaults
std::string default_namespace_name = "Res";
std::string default_resource_suffix = ".h";
std::string default_rules_file = "./res_rules.txt";

// flags
bool flags_recursive = false;

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
                prefix = rule.substr(1, spacePos-1);
                suffix = rule.substr(spacePos+1, rule.size()-1);
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

// runtime data
std::vector<FileRule> res_rules;
std::filesystem::path res_cwd{"./"};


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
        else if(arg == "--rules")
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
    std::ifstream ifstream(path, std::ios_base::binary);
    if(!ifstream.is_open())
    {
        std::cerr << "[nmres] Embed error: can't open source resource file " << path << std::endl;
        return false;
    }

    ifstream.unsetf(std::ios::skipws);

    std::streampos fileSize;
    ifstream.seekg(0, std::ios::end);
    fileSize = ifstream.tellg();
    ifstream.seekg(0, std::ios::beg);
    
    if(fileSize > 0)
    {
        std::vector<unsigned char> buffer(fileSize);
        ifstream.read((char*)buffer.data(), fileSize);

        std::string arrayname = path;
        std::replace(arrayname.begin(), arrayname.end(), '.', '_');
        std::replace(arrayname.begin(), arrayname.end(), '-', '_');
        std::replace(arrayname.begin(), arrayname.end(), '/', '_');
        std::replace(arrayname.begin(), arrayname.end(), '\\', '_');
        
        std::filesystem::path headerpath = path.concat(default_resource_suffix);
        std::ofstream ofstream(headerpath);
        if(!ofstream.is_open())
        {
            std::cerr << "[nmres] Embed error: can't open target resource file " << path << std::endl;
            return false;
        }

        ofstream << "#ifndef NMRESOURCE_EMBEDDED_RESOURCE_" << arrayname << "_H\n";
        ofstream << "#define NMRESOURCE_EMBEDDED_RESOURCE_" << arrayname << "_H\n";
        ofstream << "#pragma once\n\n";
        ofstream << "#include <array>\n";
        ofstream << "namespace " << default_namespace_name << "{\n";
        ofstream << "inline std::array<unsigned char, " << fileSize << "> " << arrayname << "_data\n";
        ofstream << "{\n";
        for(size_t i = 0; i<fileSize;)
        {
            ofstream << "\t";
            for(size_t j = 0; j<8; j++)
            {
                ofstream << "0x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned)buffer[i] << ",";
                i++;
                if(i >= fileSize)
                    break;
            }
            i++;
            if(i >= fileSize)
                break;
            ofstream << "\n";
        }
        ofstream << "\n};\n"; // array
        ofstream << "}\n"; // namespace
        ofstream << "#endif\n" << std::flush;

        std::cout << "[nmres] Embedded " << path << std::endl;
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