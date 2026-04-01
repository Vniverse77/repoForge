#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include <array>
#include <memory>
#include <cstdio>
#include <atomic>
#include <map>

namespace fs = std::filesystem;

const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string WHITE = "\033[97m";
const std::string GREEN = "\033[32m";
const std::string BLUE = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";

// [FIX 4.4] Atomic flag for safe signal handling
static std::atomic<bool> g_shouldExit{false};

void clearScreen() {
    std::system("clear");
}

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void waitForEnter() {
    std::cout << "\n" << CYAN << "Press [ENTER] to continue..." << RESET;
    std::string dummy;
    std::getline(std::cin, dummy);
}

void showExitMessage() {
    clearScreen();
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << GREEN << "  Thank you for using repoForge\n" << RESET;
    std::cout << BOLD << CYAN << "  Developer: Neuwj\n" << RESET;
    std::cout << BOLD << CYAN << "  Contact:   neuwj@bk.ru\n" << RESET;
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << YELLOW << "  Stay safe.\n\n" << RESET;
}

// [FIX 4.4] Signal handler now only sets atomic flag — no unsafe calls
void handleSigint(int sig) {
    g_shouldExit.store(true);
}

std::string execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto pipeDeleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(popen(cmd.c_str(), "r"), pipeDeleter);
    if (!pipe) return "";
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

// [FIX 4.3] isDirEmpty now reports permission errors instead of silently returning true
bool isDirEmpty(const std::string& path, bool& errorOccurred) {
    errorOccurred = false;
    try {
        if (!fs::exists(path)) return true;
        return fs::directory_iterator(path) == fs::directory_iterator();
    } catch (const std::exception& e) {
        std::cout << RED << "[-] ERROR: Cannot access directory: " << path << " — " << e.what() << RESET << "\n";
        errorOccurred = true;
        return true;
    }
}

std::string cleanPath(std::string path) {
    if (path.empty()) return path;
    size_t first = path.find_first_not_of(" \t\'\"");
    if (first == std::string::npos) return "";
    path.erase(0, first);
    path.erase(path.find_last_not_of(" \t\'\"") + 1);

    if (path.length() > 0 && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) path.replace(0, 1, std::string(home));
    }
    while (path.length() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

std::string escapeShellArg(const std::string& arg) {
    std::string escaped = "'";
    for (char c : arg) {
        if (c == '\'') escaped += "'\\''";
        else escaped += c;
    }
    escaped += "'";
    return escaped;
}

// [FIX 1.1] Sanitize strings that will be written into PKGBUILD
// Removes characters that could break bash quoting or inject commands
std::string sanitizePkgField(const std::string& input) {
    std::string sanitized;
    for (char c : input) {
        // Strip dangerous bash characters
        if (c == '"' || c == '\'' || c == '`' || c == '$' ||
            c == '\\' || c == ';' || c == '&' || c == '|' ||
            c == '(' || c == ')' || c == '{' || c == '}' ||
            c == '<' || c == '>' || c == '\n' || c == '\r') {
            continue;
            }
            sanitized += c;
    }
    return sanitized;
}

bool isValidPkgName(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_' && c != '+' && c != '.') return false;
    }
    return true;
}

// [FIX 2.4] License mapping — converts selection number to actual license name
const std::map<std::string, std::string> LICENSE_MAP = {
    {"1", "GPL"},     {"2", "GPL3"},    {"3", "MIT"},
    {"4", "Apache"},  {"5", "BSD"},     {"6", "ISC"},
    {"7", "Unlicense"}, {"8", "Python"}, {"9", "Ruby"},
    {"10", "Zlib"}
};

void printBanner() {
    clearScreen();
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << YELLOW << "   .===.   " << RESET << BOLD << "repoForge\n";
    std::cout << BOLD << YELLOW << "   | " << CYAN << "*" << YELLOW << " |   " << RESET << "Automated Repository Indexer | v0.1.0\n";
    std::cout << BOLD << YELLOW << "    \\ /    " << CYAN << "Made by Neuwj - neuwj@bk.ru\n";
    std::cout << BOLD << YELLOW << "     V     " << RESET << "\n";
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << CYAN << " System: APT, DNF, Pacman (Arch) & AUR (Docker) Ready!\n\n" << RESET;
}

void printError(const std::string& message) {
    std::cout << RED << "[-] ERROR: " << RESET << message << "\n";
}

void printStep(const std::string& message) {
    std::cout << BLUE << "[*] " << RESET << message << "...\n";
    sleepMs(400);
}

void printSuccess(const std::string& message) {
    std::cout << GREEN << "[+] " << RESET << BOLD << message << RESET << "\n";
}

std::string askInput(const std::string& question, bool isRequired = true, bool isPath = false) {
    std::string input;
    while (true) {
        std::cout << YELLOW << question << ": " << RESET;
        std::getline(std::cin, input);
        if (isPath) input = cleanPath(input);
        else {
            size_t first = input.find_first_not_of(" \t");
            if (first != std::string::npos) {
                input.erase(0, first);
                input.erase(input.find_last_not_of(" \t") + 1);
            } else input = "";
        }
        if (input.empty() && isRequired) {
            printError("This field cannot be empty.");
            continue;
        }
        return input;
    }
}


void handleAUR() {
    clearScreen();
    std::cout << BOLD << CYAN << "============================================================\n" << RESET;
    std::cout << BOLD << CYAN << "                 AUR AUTOMATION ENGINE                \n" << RESET;
    std::cout << BOLD << CYAN << "============================================================\n\n" << RESET;

    if (std::system("docker info > /dev/null 2>&1") != 0) {
        printError("Docker is not running! Please use 'sudo systemctl start docker' first.");
        waitForEnter();
        return;
    }

    const char* home = std::getenv("HOME");
    if (!home) {
        printError("HOME environment variable not set!");
        waitForEnter();
        return;
    }

    std::string sshDir = std::string(home) + "/.ssh";
    std::string pubKeyPath = "";

    printStep("Scanning for SSH Keys");
    if (fs::exists(sshDir + "/id_ed25519.pub")) pubKeyPath = sshDir + "/id_ed25519.pub";
    else if (fs::exists(sshDir + "/id_rsa.pub")) pubKeyPath = sshDir + "/id_rsa.pub";

    if (!pubKeyPath.empty()) {
        std::cout << GREEN << "[+] Found key: " << pubKeyPath << RESET << "\n";
        std::string useKey = askInput("Would you like to use this key? (Y/n)", false);
        if (useKey == "n" || useKey == "N") pubKeyPath = "";
    }

    if (pubKeyPath.empty()) {
        std::cout << YELLOW << "\nI couldn't find a suitable SSH key for AUR on your system.\n" << RESET;
        std::string createKey = askInput("Would you like me to generate one for you? (Y/n)", false);
        if (createKey != "n" && createKey != "N") {
            // [FIX 1.2] Check if key already exists before generating
            if (fs::exists(sshDir + "/id_ed25519")) {
                printError("SSH key already exists at " + sshDir + "/id_ed25519");
                std::string overwrite = askInput("Overwrite existing key? This may break other SSH connections! (y/N)", false);
                if (overwrite != "y" && overwrite != "Y") {
                    std::cout << YELLOW << "[!] Using existing key instead.\n" << RESET;
                    pubKeyPath = sshDir + "/id_ed25519.pub";
                } else {
                    // [FIX 1.3] Ask user for email instead of hardcoding
                    std::string email = askInput("Your email for SSH key comment");
                    std::string keygenCmd = "ssh-keygen -t ed25519 -C " + escapeShellArg(email) + " -f ~/.ssh/id_ed25519 -N \"\"";
                    std::system(keygenCmd.c_str());
                    pubKeyPath = sshDir + "/id_ed25519.pub";
                    printSuccess("SSH key generated successfully.");
                }
            } else {
                // [FIX 1.3] Ask user for email
                std::string email = askInput("Your email for SSH key comment");
                std::string keygenCmd = "ssh-keygen -t ed25519 -C " + escapeShellArg(email) + " -f ~/.ssh/id_ed25519 -N \"\"";
                std::system(keygenCmd.c_str());
                pubKeyPath = sshDir + "/id_ed25519.pub";
                printSuccess("SSH key generated successfully.");
            }
        } else {
            printError("AUR operations cannot proceed without an SSH key.");
            waitForEnter();
            return;
        }
    }

    if (pubKeyPath.empty()) {
        printError("No SSH key available. Cannot proceed.");
        waitForEnter();
        return;
    }

    std::cout << "\n" << BOLD << BLUE << "--- AUR ACCOUNT NOTIFICATION ---\n" << RESET;
    std::string pubKeyContent = execCommand("cat " + pubKeyPath);
    std::cout << "Please copy the following public key and add it to your AUR My Account page:\n\n";
    std::cout << BOLD << pubKeyContent << RESET << "\n\n";
    askInput(CYAN + "Press [ENTER] to continue if you have added it" + RESET, false);

    printStep("Testing AUR connection");
    std::string sshTest = execCommand("ssh -o StrictHostKeyChecking=no -T aur@aur.archlinux.org 2>&1");
    if (sshTest.find("successfully authenticated") == std::string::npos) {
        std::cout << YELLOW << "[!] Warning: Connection could not be fully verified, but proceeding...\n" << RESET;
    }

    std::cout << "\n" << BOLD << BLUE << "--- PACKAGE METADATA ENTRY ---\n" << RESET;
    std::string pkgname = askInput("Package name (pkgname)");
    if (!isValidPkgName(pkgname)) {
        printError("Invalid package name! Only alphanumeric characters, '-', '_', '+', and '.' are allowed.");
        waitForEnter();
        return;
    }

    printStep("Cloning AUR repository for " + pkgname);
    std::string cloneCmd = "git clone ssh://aur@aur.archlinux.org/" + escapeShellArg(pkgname + ".git");
    if (std::system(cloneCmd.c_str()) != 0) {
        printError("Git clone failed! Check your internet connection or SSH key authorization.");
        waitForEnter();
        return;
    }

    std::string pkgver = askInput("Version (pkgver)");
    std::string pkgdesc = askInput("Short description (pkgdesc)");
    std::string url = askInput("Project homepage (url)");
    std::string depends = askInput("Dependencies (depends) [e.g., 'gcc-libs' 'glibc']", false);
    std::string source = askInput("Source code link (.tar.gz/.zip etc.) (source)");

    // [FIX 2.1] Ask user for architecture instead of hardcoding 'any'
    std::cout << "\n" << BOLD << CYAN << "--- ARCHITECTURE SELECTION ---\n" << RESET;
    std::cout << "  [1] x86_64  (64-bit, most common)\n";
    std::cout << "  [2] any     (platform independent — scripts, data files)\n";
    std::cout << "  [3] aarch64 (ARM 64-bit)\n";
    std::string archChoice = askInput("Architecture");
    std::string arch = "x86_64"; // default
    if (archChoice == "2") arch = "any";
    else if (archChoice == "3") arch = "aarch64";

    // [FIX 2.4] License selection with proper mapping
    std::cout << "\n" << BOLD << CYAN << "--- LICENSE SELECTION ---\n" << RESET;
    std::vector<std::string> licenses = {"GPL", "GPL3", "MIT", "Apache", "BSD", "ISC", "Unlicense", "Python", "Ruby", "Zlib"};
    for (size_t i = 0; i < licenses.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << licenses[i] << "  ";
        if ((i + 1) % 5 == 0) std::cout << "\n";
    }
    std::cout << "\n";
    std::string licenseChoice = askInput("License (enter number 1-10 or custom name)");

    std::string license;
    auto it = LICENSE_MAP.find(licenseChoice);
    if (it != LICENSE_MAP.end()) {
        license = it->second;
    } else {
        license = licenseChoice; // custom license name
    }

    // [FIX 1.1] Sanitize all user inputs before writing to PKGBUILD
    std::string safePkgdesc = sanitizePkgField(pkgdesc);
    std::string safeUrl = sanitizePkgField(url);
    std::string safeLicense = sanitizePkgField(license);

    // [FIX 3.3] Download source and calculate SHA-256 with error checking
    printStep("Downloading source and calculating SHA-256");
    std::string tempFile = pkgname + "_temp";
    int wgetResult = std::system(("wget -qO " + escapeShellArg(tempFile) + " " + escapeShellArg(source)).c_str());

    std::string sha256 = "SKIP";
    if (wgetResult != 0) {
        printError("Failed to download source from: " + source);
        std::cout << YELLOW << "[!] SHA-256 will be set to 'SKIP' — package integrity won't be verified.\n" << RESET;
        std::string proceed = askInput("Continue anyway? (y/N)", false);
        if (proceed != "y" && proceed != "Y") {
            std::system(("rm -f " + escapeShellArg(tempFile)).c_str());
            waitForEnter();
            return;
        }
    } else {
        sha256 = execCommand("sha256sum " + escapeShellArg(tempFile) + " | awk '{print $1}'");
        if (sha256.empty()) sha256 = "SKIP";
    }
    // [FIX 4.2] Ensure temp file is always cleaned up
    std::system(("rm -f " + escapeShellArg(tempFile)).c_str());

    // Ask for source file name for build()
    std::string sourceFile = askInput("Main source file to compile (e.g., repoForge.cpp)");
    std::string binaryName = askInput("Output binary name (e.g., repoforge)");

    // [FIX 2.2, 2.3, 5.1] Generate a proper PKGBUILD with build() and correct package()
    printStep("Generating PKGBUILD and .SRCINFO");
    std::ofstream pkgbuild(pkgname + "/PKGBUILD");
    pkgbuild << "# Maintainer: Auto-generated by repoForge\n";
    pkgbuild << "pkgname=" << pkgname << "\n";
    pkgbuild << "pkgver=" << pkgver << "\n";
    pkgbuild << "pkgrel=1\n";
    pkgbuild << "pkgdesc=\"" << safePkgdesc << "\"\n";
    pkgbuild << "arch=('" << arch << "')\n";
    pkgbuild << "url=\"" << safeUrl << "\"\n";
    pkgbuild << "license=('" << safeLicense << "')\n";

    // [FIX 2.3] depends and source are now always written independently
    if (!depends.empty()) {
        pkgbuild << "depends=(" << depends << ")\n";
    }

    if (arch != "any") {
        pkgbuild << "makedepends=('gcc')\n";
    }

    pkgbuild << "source=(\"" << source << "\")\n";
    pkgbuild << "sha256sums=('" << sha256 << "')\n";
    pkgbuild << "\n";

    // [FIX 5.1] Add build() function for compiled packages
    if (arch != "any") {
        pkgbuild << "build() {\n";
        pkgbuild << "    cd \"$srcdir\"\n";
        pkgbuild << "    g++ -std=c++17 -O2 -Wl,-z,relro,-z,now -o " << binaryName << " " << sourceFile << " -lpthread\n";
        pkgbuild << "}\n\n";
    }

    // [FIX 2.2] package() now installs binary to /usr/bin and LICENSE if present
    pkgbuild << "package() {\n";
    pkgbuild << "    cd \"$srcdir\"\n";
    if (arch != "any") {
        pkgbuild << "    install -Dm755 " << binaryName << " \"$pkgdir/usr/bin/" << binaryName << "\"\n";
    }
    pkgbuild << "    if [ -f LICENSE ]; then\n";
    pkgbuild << "        install -Dm644 LICENSE \"$pkgdir/usr/share/licenses/$pkgname/LICENSE\"\n";
    pkgbuild << "    fi\n";
    pkgbuild << "}\n";
    pkgbuild.close();

    // [FIX 5.2] Try local makepkg first, fall back to Docker
    printStep("Generating .SRCINFO");

    bool srcInfoGenerated = false;

    // Try local first (if on Arch)
    if (std::system("command -v makepkg > /dev/null 2>&1") == 0) {
        std::string localCmd = "cd " + escapeShellArg(pkgname) + " && makepkg --printsrcinfo > .SRCINFO";
        if (std::system(localCmd.c_str()) == 0) {
            printSuccess(".SRCINFO generated locally.");
            srcInfoGenerated = true;
        }
    }

    // Fall back to Docker if local failed
    if (!srcInfoGenerated) {
        printStep("Local makepkg not available, using Docker fallback");
        std::string dockerCmd = "docker run --rm -v " + escapeShellArg(fs::current_path().string() + "/" + pkgname) + ":/pkg archlinux bash -c "
        "\"pacman -Sy --noconfirm base-devel sudo git && useradd -m builduser && chown -R builduser /pkg && "
        "sudo -u builduser bash -c 'cd /pkg && makepkg --printsrcinfo > .SRCINFO'\"";

        if (std::system(dockerCmd.c_str()) == 0) {
            printSuccess(".SRCINFO generated via Docker.");
            srcInfoGenerated = true;
        } else {
            printError("Docker process failed!");
        }
    }

    // [FIX 3.2] Don't proceed to push if .SRCINFO generation failed
    if (!srcInfoGenerated) {
        printError("Cannot proceed without .SRCINFO. Fix the errors above and try again.");
        waitForEnter();
        return;
    }

    if (askInput("Push to AUR? (Y/n)", false) != "n") {
        std::string pushCmd = "cd " + escapeShellArg(pkgname) + " && git add PKGBUILD .SRCINFO && git commit -m 'Automated push by repoForge v0.1.0' && git push origin master";
        // [FIX 3.1] Check return value of git push
        if (std::system(pushCmd.c_str()) == 0) {
            printSuccess("Package is live on AUR!");
        } else {
            printError("Git push failed! Check your SSH key and AUR permissions.");
        }
    }
    waitForEnter();
}



void initRepo() {
    std::cout << "\n" << BOLD << CYAN << "--- BUILDING REPOSITORY SKELETON ---" << RESET << "\n";
    std::string repoDir = askInput("Repository Root Directory (e.g., ~/myrepo)", true, true);
    try {
        fs::create_directories(repoDir + "/debian/pool/main");
        fs::create_directories(repoDir + "/rpm");
        fs::create_directories(repoDir + "/keys");
        std::ofstream gitignore(repoDir + "/.gitignore");
        gitignore << "# Security\n*.sec\n*.sk\n\n# Temp\n*.log\n.repodata.old/\n";
        printSuccess("Skeleton ready at: " + repoDir);
    } catch (const std::exception& e) { printError(e.what()); }
    waitForEnter();
}

void indexRepo() {
    std::cout << "\n" << BOLD << CYAN << "--- SCANNING & INDEXING ENGINE ---" << RESET << "\n";
    std::string repoDir = askInput("Repository Root Directory", true, true);
    if (!fs::exists(repoDir)) { printError("Directory not found!"); waitForEnter(); return; }

    bool dirError = false;

    // Debian indexing
    std::string debianPool = repoDir + "/debian/pool/main";
    if (fs::exists(debianPool) && !isDirEmpty(debianPool, dirError)) {
        printStep("Indexing Debian packages");
        std::string codename = askInput("Codename (leave empty for 'stable')", false); if(codename.empty()) codename = "stable";
        std::string debArch = askInput("Architecture (leave empty for 'amd64')", false); if(debArch.empty()) debArch = "amd64";
        std::string distsDir = repoDir + "/debian/dists/" + codename + "/main/binary-" + debArch;
        fs::create_directories(distsDir);

        std::string cmd = "cd " + escapeShellArg(repoDir + "/debian") + " && dpkg-scanpackages pool/main /dev/null > " + escapeShellArg("dists/" + codename + "/main/binary-" + debArch + "/Packages");
        if (std::system(cmd.c_str()) == 0) {
            std::system(("gzip -k -f " + escapeShellArg(distsDir + "/Packages")).c_str());
            printSuccess("Debian index updated.");
        } else {
            printError("Debian indexing failed! Is dpkg-dev installed?");
        }
    } else if (dirError) {
        printError("Skipping Debian indexing due to directory access error.");
    }

    // [FIX 5.3] RPM indexing
    std::string rpmDir = repoDir + "/rpm";
    if (fs::exists(rpmDir) && !isDirEmpty(rpmDir, dirError)) {
        printStep("Indexing RPM packages");
        std::string rpmCmd = "createrepo_c " + escapeShellArg(rpmDir);
        if (std::system(rpmCmd.c_str()) == 0) {
            printSuccess("RPM index updated.");
        } else {
            printError("RPM indexing failed! Is createrepo_c installed?");
        }
    } else if (dirError) {
        printError("Skipping RPM indexing due to directory access error.");
    }

    waitForEnter();
}

void installTools() {
    std::cout << "\n" << BOLD << CYAN << "--- INSTALLING REQUIRED TOOLS ---" << RESET << "\n";
    bool hasApt = (std::system("command -v apt > /dev/null 2>&1") == 0);
    bool hasDnf = (std::system("command -v dnf > /dev/null 2>&1") == 0);
    bool hasPacman = (std::system("command -v pacman > /dev/null 2>&1") == 0);

    int result = -1;

    if (hasApt) {
        printStep("Debian/Ubuntu/MX Linux detected");
        result = std::system("sudo apt update && sudo apt install -y dpkg-dev apt-utils createrepo-c docker.io git openssh-client");
    } else if (hasDnf) {
        printStep("Fedora/RHEL detected");
        result = std::system("sudo dnf install -y createrepo_c dpkg docker git openssh-clients");
    } else if (hasPacman) {
        printStep("Arch Linux detected");
        result = std::system("sudo pacman -Sy --noconfirm dpkg createrepo_c docker git openssh");
    } else {
        printError("No supported package manager found (apt, dnf, or pacman).");
        waitForEnter();
        return;
    }

    // [FIX 3.1] Check return value before printing success
    if (result == 0) {
        printSuccess("All tools installed successfully. Don't forget to start Docker if needed.");
    } else {
        printError("Some packages may have failed to install. Check the output above.");
    }
    waitForEnter();
}

void showGuide() {
    clearScreen();
    std::cout << BOLD << CYAN << "--- USER GUIDE ---\n" << RESET;
    std::cout << "1. Init: Creates repository subdirectories (debian/pool, rpm, keys).\n";
    std::cout << "2. Index: Scans and indexes .deb and .rpm packages.\n";
    std::cout << "3. AUR: Prepares and pushes an Arch package (local or Docker).\n";
    std::cout << "4. Install: Installs required tools for your distro.\n";
    askInput("Press [ENTER] to return", false);
}

int main() {
    std::signal(SIGINT, handleSigint);

    while(true) {
        // [FIX 4.4] Check atomic flag for clean exit
        if (g_shouldExit.load()) {
            showExitMessage();
            break;
        }

        printBanner();
        std::cout << "  [1] Create Repo Skeleton\n  [2] Scan and Index Repository\n  [3] AUR Automation Engine\n  [4] Install Required Tools\n  [0] User Guide\n  [q] Exit\n\n";
        std::string choice = askInput("Your choice", true, false);
        if (choice == "1") initRepo();
        else if (choice == "2") indexRepo();
        else if (choice == "3") handleAUR();
        else if (choice == "4") installTools();
        else if (choice == "0") showGuide();
        else if (choice == "q" || choice == "Q") { showExitMessage(); break; }
    }
    return 0;
}
