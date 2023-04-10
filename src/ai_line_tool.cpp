#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib> // for system()
#include <unistd.h>
#include <limits.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/utsname.h>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool logging_enabled = false;
std::ofstream log_file;

std::string read_api_key_from_file(const std::string& filename) {
    std::ifstream config_file(filename);
    if (config_file.is_open()) {
        std::string local_api_key; // Use a local variable instead
        getline(config_file, local_api_key);
        config_file.close();
        //std::cout << "API key read from file: " << local_api_key << std::endl;
        return local_api_key;
    }
    return "";
}


void write_api_key_to_file(const std::string& filename, const std::string& api_key) {
    std::ofstream config_file(filename);
    if (config_file.is_open()) {
        config_file << api_key;
        config_file.close();
        //std::cout << "API key written to file: " << api_key << std::endl;
    } else {
        std::cerr << "Unable to create config file: " << filename << std::endl;
    }
}

size_t callback(const char* in, size_t size, size_t num, std::string* out) {
    const size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

std::string get_system_info() {
    struct utsname sysinfo;
    if (uname(&sysinfo) == -1) {
        perror("uname");
        return "";
    }

    std::string system_info = "System: ";
    system_info += sysinfo.sysname;
    system_info += ", Version: ";
    system_info += sysinfo.release;
    system_info += ", Machine: ";
    system_info += sysinfo.machine;

    return system_info;
}


std::string chat_with_gpt(const json& messages, const std::string& api_key) {
    std::string response_str;
    json data = {
        {"model", "gpt-4"},
        {"max_tokens", 50},
        {"messages", messages}
    };

    std::string url = "https://api.openai.com/v1/chat/completions";
    std::string auth_header = "Authorization: Bearer " + api_key;

    // Print the request data for debugging
    //std::cout << "Request: " << data.dump() << std::endl;
    if (logging_enabled && log_file.is_open()) {
        log_file << "Request: " << data.dump() << std::endl;
    }

    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = nullptr;
        std::string json = data.dump();

        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    json response = json::parse(response_str);

    if (logging_enabled && log_file.is_open()) {
        log_file << "Response: " << response.dump() << std::endl;
    }
    // Print the response data for debugging
    //std::cout << "Response: " << response.dump() << std::endl;

    if (response["choices"][0]["message"]["content"].is_null()) {
        return "";
    } else {
        return response["choices"][0]["message"]["content"].get<std::string>();
    }
}

bool change_directory(const std::string& command, std::string& current_path) {
    std::istringstream iss(command);
    std::string cmd, path;

    iss >> cmd;
    std::getline(iss, path);

    // Remove leading and trailing spaces from the path
    path.erase(0, path.find_first_not_of(' '));
    path.erase(path.find_last_not_of(' ') + 1);

    if (cmd == "cd") {
        if (!path.empty()) {
            // Replace ~ with the user's home directory
            if (path[0] == '~') {
                const char* home_dir = getenv("HOME");
                if (home_dir != nullptr) {
                    path.replace(0, 1, home_dir);
                } else {
                    std::cout << "Error: Failed to get the home directory." << std::endl;
                    return false;
                }
            }

            if (chdir(path.c_str()) == 0) {
                char buffer[PATH_MAX];
                if (getcwd(buffer, sizeof(buffer)) != nullptr) {
                    current_path = buffer;
                    return true;
                }
            } else {
                std::cout << "Error: Failed to change directory." << std::endl;
                return false;
            }
        } else {
            std::cout << "Error: Missing directory path for 'cd' command." << std::endl;
            return false;
        }
    }
    return false;
}





std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}


std::string exec_and_get_output(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

std::string execute_command(const std::string& command) {
    std::string output;
    FILE* fp = popen(command.c_str(), "r");
    if (fp != nullptr) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            output += buffer;
        }
        pclose(fp);
    }
    return output;
}

int estimate_token_count(const json& message) {
    std::string content = message["content"].get<std::string>();
    return content.size() + 1; // Add 1 to account for the space between words
}

void trim_conversation(json& messages, int token_limit) {
    int total_tokens = 0;
    for (const auto& message : messages) {
        total_tokens += estimate_token_count(message);
    }

    while (total_tokens > token_limit && messages.size() > 1) {
        total_tokens -= estimate_token_count(messages[1]);
        messages.erase(messages.begin() + 1);
    }
}



int main() {
    std::string config_filename = "config.txt";
    std::string api_key = read_api_key_from_file(config_filename);

    if (api_key.empty()) {
        std::cout << "Please enter your OpenAI API key (must have access to GPT-4): ";
        getline(std::cin, api_key);

        if (!api_key.empty()) {
            write_api_key_to_file(config_filename, api_key);
        } else {
            std::cerr << "Invalid API key" << std::endl;
            return 1;
        }
    }

    //std::cout << "API key: " << api_key << std::endl; // Log the API key

    std::string os_type = "Linux";
#ifdef _WIN32
    os_type = "Windows";
#endif

    std::cout << "Welcome to the " << os_type << " command-line helper." << std::endl;
    std::cout << "Type a command in human language or type 'exit' to quit." << std::endl;
    std::cout << "Some examples:" << std::endl;
    std::cout << "-Where am I?" << std::endl;
    std::cout << "-Go to that Document folder from earlier and show me the content" << std::endl;
    std::cout << "-Create a text file readme.txt and put the content 'Call me, Chris' in the file. Change owner to root." << std::endl;

    std::string system_info = get_system_info();

    json messages = {
        {{"role", "assistant"}, {"content", "You are an AI assistant that help users with " + system_info + " command-line tasks on " + os_type + " systems. Please respond only with the relevant command.  If needed you can respond with multiple commands separated by linebreak. If you are unsure what the user is asking, you can reply by saying {{clarify}}"}}
    };

    std::string current_path;
    {
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            current_path = buffer;
        } else {
            std::cerr << "Error: Failed to get the current working directory." << std::endl;
            return 1;
        }
    }

    std::vector<std::string> commands; // Declare 'commands' here  

    while (true) {
        std::string user_input;
        std::cout << "user@" << os_type << ":" << current_path << "$ ";
        std::getline(std::cin, user_input);

        if (user_input == "exit") {
            break;
        }

        if (user_input == "log on") {
            logging_enabled = true;
            log_file.open("command_log.txt", std::ios::app);
            std::cout << "Logging enabled." << std::endl;
            continue;
        }

        if (user_input == "log off") {
            logging_enabled = false;
            if (log_file.is_open()) {
                log_file.close();
            }
            std::cout << "Logging disabled." << std::endl;
            continue;
        }

        messages.push_back({{"role", "user"}, {"content", user_input}});

        trim_conversation(messages, 3096);
        std::string ai_response = chat_with_gpt(messages, api_key); // Pass the api_key as an argument

        messages.push_back({{"role", "assistant"}, {"content", ai_response}});
        commands = split_string(ai_response, '\n');

        for (const std::string& command : commands) {
            std::cout << "Command: " << command << std::endl;
            if (!change_directory(command, current_path)) {
                std::string cmd_output = execute_command(command);
                if (!cmd_output.empty()) {
                    std::cout << "Output: " << cmd_output << std::endl;
                    messages.back()["content"] = messages.back()["content"].get<std::string>() + "\n\n" + cmd_output;
                } else {
                    std::cout << "Error: command execution returned empty output." << std::endl;
                }

                if (logging_enabled && log_file.is_open()) {
                    log_file << "Command: " << command << std::endl;
                    log_file << "Output: " << cmd_output << std::endl;
                }
            }
        }

    }

    return 0;
}


