#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;


class utils {
public:
    // Entry: parse and dispatch
    void executeCommand(const std::string& input) {
        std::vector<std::string> words = split(input);
        execute(words);
    }

    // Help text (deduplicated)
    void help() {
        std::cout
            << "Available commands:\n"
            << "  help             : Display available commands.\n"
            << "  exit             : Exit the shell.\n"
            << "  ls [-a]          : List files in the current directory.\n"
            << "  pwd              : Print working directory.\n"
            << "  cd [dir]         : Change current directory (default HOME).\n"
            << "  rm [-r] <path>   : Remove file or directory (recursive with -r).\n"
            << "  cp <src> <dst>   : Copy file.\n"
            << "  mv <src> <dst>   : Move/rename.\n"
            << "  mkdir [-m OCT] d : Create directory (optional octal perms e.g. 755).\n"
            << "  touch <file>     : Create empty file or update timestamp.\n"
            << "  man [cmd]        : Show manual/command list or details.\n"
            << "  clear            : Clear terminal.\n"
            << "  nano <file>      : Open a text editor for file.\n"
            << "  rename <s> <d>   : Rename file or directory. (alias of mv)\n"
            << "  exec <file>      : Execute .c/.cpp/.py/.sh source (compile/run).\n"
            << "  run <script.sh>  : Run shell script line by line inside this shell.\n"
            << std::flush;
    }

    // Command list for man
    void show_all_commands(){
        static const char* commands[] = {
            "ls: List files and directories in the current working directory.",
            "pwd: Print the current working directory.",
            "cd: Change the current working directory.",
            "cp: Copy files.",
            "rm: Remove files or directories.",
            "man: Display manual for a command.",
            "mv: Move or rename files or directories.",
            "mkdir: Create a new directory.",
            "touch: Create an empty file or update timestamps.",
            "chmod: Change file permissions (not implemented as command, use mkdir -m).",
            "exit: Exit the shell.",
            "echo: Print arguments.",
            "cat: Display contents of files (not implemented).",
            ">>: Append output to a file (use OS redirection).",
            "<<: Here document (use OS shell).",
            "history: Show recent commands (not implemented).",
            "grep: Search patterns in files (use OS grep).",
            "wget: Download files (external tool).",
            "curl: Transfer data (external tool).",
            "unzip: Extract ZIP archives (external tool)."
        };
        std::cout << "Command Manual:\n";
        for (auto s : commands) std::cout << s << "\n";
    }

    void manualCommand(const std::string& flags) {
        if (flags.empty()) { show_all_commands(); return; }
        // Minimal per-command blurb
        if (flags == "ls")        std::cout << "ls: List files. Use -a to include hidden.\n";
        else if (flags == "pwd")  std::cout << "pwd: Print working directory.\n";
        else if (flags == "cd")   std::cout << "cd [dir]: Change directory. Empty -> HOME.\n";
        else if (flags == "rm")   std::cout << "rm [-r] <path>: Remove file/dir.\n";
        else if (flags == "cp")   std::cout << "cp <src> <dst>: Copy file.\n";
        else if (flags == "mv" || flags == "rename") std::cout << "mv/rename <src> <dst>: Move or rename.\n";
        else if (flags == "mkdir")std::cout << "mkdir [-m OCT] <dir>: Create directory with optional octal perms (e.g. 755).\n";
        else if (flags == "touch")std::cout << "touch <file>: Create or update timestamp.\n";
        else if (flags == "exec") std::cout << "exec <file>: Execute .c/.cpp/.py/.sh.\n";
        else if (flags == "run")  std::cout << "run <script.sh>: Execute script lines in this shell.\n";
        else if (flags == "nano") std::cout << "nano <file>: Open text editor (platform-aware).\n";
        else help();
    }

    // mkdir
    void makedir(const std::string& path){
        try {
            if (!fs::create_directory(path))
                std::cout << "Could not create directory or already exists: " << path << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
        }
    }

    // rm
    void rm(const std::vector<std::string>& ip) {
        bool recursive = false;
        std::string fpath;
        for (size_t i = 1; i < ip.size(); ++i) {
            if (ip[i] == "-r") recursive = true; else fpath = ip[i];
        }
        if (fpath.empty()) { std::cout << "Usage: rm [-r] <path>\n"; return; }
        try {
            if (recursive) {
                auto count = fs::remove_all(fpath);
                if (count > 0) std::cout << "Removed recursively: " << fpath << " (" << count << ")\n";
                else std::cerr << "Nothing removed: " << fpath << "\n";
            } else {
                if (fs::remove(fpath)) std::cout << "Removed: " << fpath << "\n";
                else std::cerr << "Failed to remove: " << fpath << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error removing: " << e.what() << "\n";
        }
    }

    // cd
    void cd(const std::string& dir) {
        try {
            if (dir.empty()) {
                const char* home = std::getenv("HOME");
                if (home) fs::current_path(home); else std::cerr << "HOME not set\n";
            } else {
                fs::current_path(dir);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error changing directory: " << e.what() << "\n";
        }
    }

    // mkdir with octal permissions like 755
    void makedirwithpermissions(const std::string& path, const std::string& oct) {
        try {
            if (!fs::create_directory(path)) {
                std::cerr << "Could not create directory or already exists: " << path << "\n";
                return;
            }
            apply_octal_permissions(path, oct);
        } catch (const std::exception& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
        }
    }

    // Platform-aware editor
    void opennotepad(const std::string& filename){
#ifdef _WIN32
        std::string cmd = "notepad \"" + filename + "\"";
#elif __APPLE__
        std::string cmd = "open -e \"" + filename + "\""; // TextEdit
#else
        std::string cmd = "nano \"" + filename + "\"";    // CLI editor
#endif
        std::system(cmd.c_str());
    }

    // clear
    void clearfunc(){ std::cout << "\033[2J\033[1;1H"; }

    // copy file
    void copyfunction(const std::string& source, const std::string& destination) {
        try {
            std::ifstream src(source, std::ios::binary);
            std::ofstream dst(destination, std::ios::binary);
            if (src && dst) { dst << src.rdbuf(); std::cout << "File copied.\n"; }
            else std::cerr << "Error: Could not open files.\n";
        } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << "\n"; }
    }

    // dispatcher
    void execute(const std::vector<std::string>& input) {
        if (input.empty()) return;
        const std::string& cmd = input[0];

        if (cmd == "help") help();
        else if (cmd == "ls") { bool showHidden = (input.size() > 1 && input[1] == "-a"); listDirectory(".", showHidden); }
        else if (cmd == "pwd") std::cout << "Present working directory: " << pwd() << "\n";
        else if (cmd == "cd") cd(input.size() > 1 ? input[1] : "");
        else if (cmd == "rm") rm(input);
        else if (cmd == "mkdir") {
            if (input.size() == 2) makedir(input[1]);
            else if (input.size() == 4 && input[1] == "-m") makedirwithpermissions(input[3], input[2]);
            else std::cout << "Usage: mkdir [-m OCTAL] <directory>\n";
        }
        else if (cmd == "touch") {
            if (input.size() > 1) touchFile(input[1]); else std::cout << "Usage: touch <file>\n";
        }
        else if (cmd == "clear") clearfunc();
        else if (cmd == "nano") {
            if (input.size() > 1) opennotepad(input[1]); else std::cout << "Usage: nano <file>\n";
        }
        else if (cmd == "rename" || cmd == "mv") {
            if (input.size() > 2) renameFile(input[1], input[2]); else std::cout << "Usage: mv <src> <dst>\n";
        }
        else if (cmd == "cp") {
            if (input.size() > 2) copyfunction(input[1], input[2]); else std::cout << "Usage: cp <src> <dst>\n";
        }
        else if (cmd == "man") manualCommand(input.size() > 1 ? input[1] : "");
        else if (cmd == "exec") {
            if (input.size() > 1) exec(input[1]); else std::cout << "Usage: exec <path_to_file>\n";
        }
        else if (cmd == "run") {
            if (input.size() > 1) run(input[1]); else std::cout << "Usage: run <path_to_sh_file>\n";
        }
        else std::cout << "Unknown command: " << cmd << "\n";
    }

    // touch: create or update timestamp
    void touchFile(const std::string& filename) {
        fs::path p = filename;
        // ensure parent exists if a path like a/b.txt given
        if (!p.parent_path().empty()) fs::create_directories(p.parent_path());
        // create if not exists
        std::ofstream(p, std::ios::app).close();
        // update mtime
        try { fs::last_write_time(p, fs::file_time_type::clock::now()); } catch(...) {}
    }

    void renameFile(const std::string& source, const std::string& destination) {
        try {
            if (fs::exists(source)) fs::rename(source, destination);
            else std::cout << "Source does not exist.\n";
        } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << "\n"; }
    }

    void listDirectory(const std::string& directory, bool showHidden) {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                std::string filename = entry.path().filename().string();
                if (!showHidden && !filename.empty() && filename[0] == '.') continue;
                std::cout << filename << ' ';
            }
            std::cout << '\n';
        } catch (const std::exception& e) { std::cerr << "Error listing directory: " << e.what() << "\n"; }
    }

    std::vector<std::string> split(const std::string& line) {
        std::istringstream iss(line);
        std::vector<std::string> words; std::string w;
        while (iss >> w) words.push_back(w);
        return words;
    }

    // Run .sh script line by line in this shell
    void run(const std::string& p) {
        std::ifstream file(p);
        if (!file) { std::cerr << "Cannot open script: " << p << "\n"; return; }
        std::string lin;
        while (std::getline(file, lin)) {
            // skip empty and comments
            std::string trimmed = trim(lin);
            if (trimmed.empty() || trimmed[0] == '#') continue;
            executeCommand(lin);
        }
    }

    std::string pwd() {
        try { return fs::current_path().string(); }
        catch (const std::exception& e) { std::cerr << "Error getting cwd: " << e.what() << "\n"; return ""; }
    }

    void exec(const std::string& path) {
        if (!fs::exists(path)) { std::cout << "File not found: " << path << "\n"; return; }
        std::string ext;
        auto dot = path.find_last_of('.');
        if (dot != std::string::npos) ext = path.substr(dot + 1);

        if (ext == "cpp") cpp(path);
        else if (ext == "c") executeCFile(path);
        else if (ext == "py") executePythonFile(path);
        else if (ext == "sh") {
            std::string cmd = std::string("bash \"") + path + "\"";
            std::system(cmd.c_str());
        }
        else std::cout << "Cannot execute this file type.\n";
    }

    void cpp(const std::string& pa) {
        fs::path p(pa); std::string exep = p.stem().string();
        std::string comp = "g++ -std=c++17 -O2 -o \"" + exep + "\" \"" + pa + "\"";
        int res = std::system(comp.c_str());
        if (res == 0) run_binary(exep); else std::cout << "Compilation failed.\n";
    }

    void executeCFile(const std::string& pa) {
        fs::path p(pa); std::string exep = p.stem().string();
        std::string comp = "gcc -O2 -o \"" + exep + "\" \"" + pa + "\"";
        int res = std::system(comp.c_str());
        if (res == 0) run_binary(exep); else std::cout << "Compilation failed.\n";
    }

    void executePythonFile(const std::string& pyFilePath) {
        // try python3 then python
        int r = std::system((std::string("python3 \"") + pyFilePath + "\"").c_str());
        if (r != 0) r = std::system((std::string("python \"") + pyFilePath + "\"").c_str());
        if (r != 0) std::cout << "Execution failed.\n";
    }

private:
    static std::string trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    void run_binary(const std::string& name) {
#ifdef _WIN32
        std::string cmd = "\"" + name + "\"";
#else
        std::string cmd = "./" + name;
#endif
        std::system(cmd.c_str());
    }

    void apply_octal_permissions(const std::string& path, const std::string& oct) {
        // Expect 3-digit octal like 755
        if (oct.size() != 3 || !std::all_of(oct.begin(), oct.end(), ::isdigit)) return;
        int o = std::stoi(oct, nullptr, 8);
        fs::perms perms = fs::perms::none;
        // owner/group/others bits
        if (o & 0400) perms |= fs::perms::owner_read;
        if (o & 0200) perms |= fs::perms::owner_write;
        if (o & 0100) perms |= fs::perms::owner_exec;
        if (o & 0040) perms |= fs::perms::group_read;
        if (o & 0020) perms |= fs::perms::group_write;
        if (o & 0010) perms |= fs::perms::group_exec;
        if (o & 0004) perms |= fs::perms::others_read;
        if (o & 0002) perms |= fs::perms::others_write;
        if (o & 0001) perms |= fs::perms::others_exec;
        try { fs::permissions(path, perms); } catch(...) {}
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    utils obj;
    std::string input;
    std::cout << "Iron Shell Interface\nType 'help' to see available commands.\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) break;
        if (input == "exit") { std::cout << "Exiting shell\n"; return 0; }
        obj.executeCommand(input);
    }
    return 0;
}
