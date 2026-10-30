#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXNetSystem.h> 
#include <ixwebsocket/IXWebSocketServer.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex> 
#include <set>


namespace fs = std::filesystem;

// Document block types
enum class BlockType {
    Title,
    Header,
    Subheader,
    Text,
    Diagram,
    Math,
    Graph,
    Caption,
    Explanation
};

struct ContentBlock {
    BlockType type;
    std::string content;
    std::string metadata; // e.g., image path or graph title
};

class DocumentParser {
public:
    static std::vector<ContentBlock> parseFile(const fs::path& filePath) {
        std::vector<ContentBlock> blocks;
        std::ifstream file(filePath);
        if (!file.is_open()) return blocks;

        std::string line;
        BlockType currentType = BlockType::Text;
        std::string currentContent = "";
        std::string currentMetadata = "";

        auto flushBlock = [&]() {
            if (!currentContent.empty() || currentType != BlockType::Text) {
                blocks.push_back({currentType, currentContent, currentMetadata});
                currentContent.clear();
                currentMetadata.clear();
            }
        };

        // 2. Updated DocumentParser::parseFile delimiter detection
        while (std::getline(file, line)) {
            if (line.rfind("[TITLE]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Title;
            } 
            else if (line.rfind("[HEADER]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Header;
            } 
            else if (line.rfind("[SUBHEADER]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Subheader;
            } 
            else if (line.rfind("[TEXT]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Text;
            } 
            else if (line.rfind("[DIAGRAM:", 0) == 0) {
                flushBlock();
                currentType = BlockType::Diagram;
                currentMetadata = line.substr(9, line.find(']') - 9);
            } 
            else if (line.rfind("[MATH]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Math;
            } 
            else if (line.rfind("[GRAPH]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Graph;
            } 
            else if (line.rfind("[CAPTION]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Caption;
            } 
            else if (line.rfind("[EXPLANATION]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Explanation;
            } 
            else {
                currentContent += line + "\n";
            }
        }
        flushBlock();
        return blocks;
    }
};


class HtmlRenderer {
public:
    static std::string render(const std::vector<ContentBlock>& blocks, const fs::path& baseDir) {
        // Load index.html template alongside text file or use default corporate light fallback
        fs::path templatePath = baseDir / "index.html";
        std::string htmlTemplate;

        if (fs::exists(templatePath)) {
            std::ifstream templateFile(templatePath);
            std::stringstream buffer;
            buffer << templateFile.rdbuf();
            htmlTemplate = buffer.str();
        } 
        else {
            // Fallback template linking external style.css and script.js if index.html is missing
            htmlTemplate = "<!DOCTYPE html><html><head><link rel='stylesheet' href='themes.css'>"
                           "</head><body>{{CONTENT}}<script src='app.js'></script></body></html>";
        }

        // Render each content block with corporate corporate light styling tags
        std::stringstream contentStream;
        for (const auto& block : blocks) {
            switch (block.type) {
                case BlockType::Title:
                    contentStream << "<h1 class='corp-title'>" << block.content << "</h1>\n";
                    break;
                case BlockType::Header:
                    contentStream << "<h2 class='corp-header'>" << block.content << "</h2>\n";
                    break;
                case BlockType::Subheader:
                    contentStream << "<h3 class='corp-subheader'>" << block.content << "</h3>\n";
                    break; 
                case BlockType::Text: {
                    std::stringstream textStream(block.content);
                    std::string tLine;
                    bool inList = false; 
                    contentStream << "<div class='text-block'>\n";
                    while (std::getline(textStream, tLine)) {
                        // Check if line represents a point-form list item (e.g., "- Some point.")
                        if (tLine.rfind("- ", 0) == 0) {
                            if (!inList) {
                                contentStream << "<ul style='list-style-type: disc; margin-left: 20px; margin-bottom: 16px; color: #334155;'>\n";
                                inList = true;
                            }
                            contentStream << "<li style='margin-bottom: 6px;'>" << tLine.substr(2) << "</li>\n";
                        } 
                        else {
                            if (inList) {
                                contentStream << "</ul>\n";
                                inList = false;
                            }
                            if (!tLine.empty()) {
                                contentStream << "<p class='corp-text'>" << tLine << "</p>\n";
                            }
                        }
                    }
                    if (inList) {
                        contentStream << "</ul>\n";
                    }
                    contentStream << "</div>\n";
                    break;
                }                   
                case BlockType::Diagram: {
                    fs::path imgPath = baseDir / block.metadata;
                    contentStream << "<div class='diagram-block'>\n";
                    if (fs::exists(imgPath)) {
                        contentStream << "<img src=\"/file/" << imgPath.generic_string() << "\" alt=\"Diagram\">\n";
                    } 
                    else {
                        contentStream << "<p style='color:red;'>[Diagram file missing: " << block.metadata << "]</p>\n";
                    }
                    contentStream << "</div>\n";
                    break;
                }
                case BlockType::Math:
                    contentStream << "<div class='corp-math'>$$" << block.content << "$$</div>\n";
                    break;
                case BlockType::Graph: {
                    contentStream << "<div class='graph-block'>"<< "<table class='corp-table'>"; 
                    std::stringstream ss(block.content);
                    std::string rowLine;
                    bool isHeader = true;
                    while (std::getline(ss, rowLine)) {
                        if (rowLine.empty()) {
                            continue;
                        } 
                        contentStream << "<tr>"; 
                        std::stringstream rowStream(rowLine);
                        std::string cell; 
                        while (std::getline(rowStream, cell, ',')) {
                            if (isHeader) {
                                contentStream << "<th>" << cell << "</th>";
                            } 
                            else {
                                contentStream << "<td>" << cell << "</td>";
                            }
                        } 
                        contentStream << "</tr>\n";
                        isHeader = false;
                    } 
                    contentStream << "</table></div>\n";
                    break;    
                }                     
                case BlockType::Caption:
                    contentStream << "<div class='corp-caption'>" << block.content << "</div>\n";
                    break;
                case BlockType::Explanation:
                    contentStream << "<div class='corp-explanation'><strong>Variables:</strong> " << block.content << "</div>\n";
                    break;
            }
        }

        // Replace template placeholder with rendered blocks
        size_t placeholderPos = htmlTemplate.find("{{CONTENT}}");
        if (placeholderPos != std::string::npos) {
            htmlTemplate.replace(placeholderPos, 11, contentStream.str());
        } 
        else {
            htmlTemplate += contentStream.str();
        }

        return htmlTemplate;
    }
}; 

int main() {
    ix::initNetSystem();

    int httpPort = 9002;
    int wsPort = 9003;

    ix::HttpServer httpServer(httpPort, "0.0.0.0"); 
    //    ix::HttpServer server(port, "0.0.0.0"); 
    ix::WebSocketServer wsServer(wsPort, "0.0.0.0");

    #ifdef SECUREDOC_DOCUMENT_ROOT
        fs::path rootDir = SECUREDOC_DOCUMENT_ROOT;
    #else
        fs::path rootDir = fs::absolute("./documents");
    #endif
   
   // HTTP Route Handling (Port 9002)
    httpServer.setOnConnectionCallback([rootDir](std::shared_ptr<ix::HttpRequest> request, std::shared_ptr<ix::ConnectionState> connectionState) -> std::shared_ptr<ix::HttpResponse> {
        std::string uri = request->uri;
        fs::path targetFile;

        // 1. Handle document viewing routes (/view/...)
        if (uri.rfind("/view/", 0) == 0) {
            std::string relativePath = uri.substr(6);
            targetFile = rootDir / relativePath;

            if (fs::exists(targetFile) && fs::is_regular_file(targetFile)) {
                auto blocks = DocumentParser::parseFile(targetFile);
                std::string htmlOutput = HtmlRenderer::render(blocks, targetFile.parent_path());
                      
                auto response = std::make_shared<ix::HttpResponse>();
                response->statusCode = 200;
                response->headers["Content-Type"] = "text/html; charset=utf-8";
                response->headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0";
                response->headers["Pragma"] = "no-cache";
                response->body = htmlOutput;
                return response;
            }
        }
        // 2. Handle static assets (theme.css, script.js, images, index.html)
        else {
            // Remove leading slash if present
            std::string assetPath = (uri == "/" || uri.empty()) ? "index.html" : uri.substr(1);
            // If requested via /file/ path prefix
            if (assetPath.rfind("file/", 0) == 0) {
                assetPath = assetPath.substr(5);
            }
            targetFile = rootDir / assetPath;

            if (fs::exists(targetFile) && fs::is_regular_file(targetFile)) {
                auto response = std::make_shared<ix::HttpResponse>();
                response->statusCode = 200;
                
                // Set appropriate MIME types
                std::string ext = targetFile.extension().string();
                if (ext == ".css") response->headers["Content-Type"] = "text/css; charset=utf-8";
                else if (ext == ".js") response->headers["Content-Type"] = "application/javascript; charset=utf-8";
                else if (ext == ".png") response->headers["Content-Type"] = "image/png";
                else if (ext == ".jpg" || ext == ".jpeg") response->headers["Content-Type"] = "image/jpeg"; 
                else if (ext == ".svg") response->headers["Content-Type"] = "image/svg+xml";
                else response->headers["Content-Type"] = "text/plain; charset=utf-8";

                // Read binary or text file content
                std::ifstream file(targetFile, std::ios::binary);
                std::stringstream buffer;
                buffer << file.rdbuf();
                response->body = buffer.str();
                return response;
            }
        }

        auto response = std::make_shared<ix::HttpResponse>();
        response->statusCode = 404;
        response->body = "Resource Not Found: " + uri;
        return response;
    }); 
    
    // Track active WebSocket clients for broadcasting changes
    std::mutex wsClientsMutex;
    std::set<std::shared_ptr<ix::WebSocket>> wsClients;

    // WebSocket Server for Live Reloads (Port 9003)
    wsServer.setOnConnectionCallback([&wsClients, &wsClientsMutex](std::weak_ptr<ix::WebSocket> webSocketWeak, std::shared_ptr<ix::ConnectionState> connectionState) {
        // Lock weak_ptr to shared_ptr inside message callbacks
        auto webSocket = webSocketWeak.lock();
        if (!webSocket) return;

        webSocket->setOnMessageCallback([webSocketWeak, &wsClients, &wsClientsMutex](const ix::WebSocketMessagePtr& msg) {
            auto webSocket = webSocketWeak.lock();
            if (!webSocket) return;

            if (msg->type == ix::WebSocketMessageType::Open) {
                std::lock_guard<std::mutex> lock(wsClientsMutex);
                wsClients.insert(webSocket);
            } 
            else if (msg->type == ix::WebSocketMessageType::Close) {
                std::lock_guard<std::mutex> lock(wsClientsMutex);
                wsClients.erase(webSocket);
            }
        });
    });
    
    // Start servers
    //httpServer.listen(); 
    auto httpResponse = httpServer.listen(); 
    if (!httpResponse.first) {
        std::cerr << "Error initializing server on port " << httpPort << ": " << httpResponse.second << std::endl;
        return 1;
    }
    httpServer.start();
    
    //wsServer.listen();
    auto wsResponse = wsServer.listen(); 
    if (!wsResponse.first) {
        std::cerr << "Error initializing server on port " << httpPort << ": " << wsResponse.second << std::endl;
        return 1;
    }
    wsServer.start();

    std::cout << "HTTP Server running on port " << httpPort << "\n";
    std::cout << "Live-Reload WebSocket Server running on port " << wsPort << "\n";
   
   // Background File Watcher Thread (Polls every 500ms)
    std::thread fileWatcher([&rootDir, &wsClients, &wsClientsMutex]() {
        std::unordered_map<std::string, fs::file_time_type> fileTimes;

        while (true) {
            if (fs::exists(rootDir)) {
                for (auto& p : fs::recursive_directory_iterator(rootDir)) {
                    if (fs::is_regular_file(p)) {
                        auto currentWriteTime = fs::last_write_time(p);
                        std::string pathStr = p.path().generic_string();

                        if (fileTimes.find(pathStr) != fileTimes.end()) {
                            if (fileTimes[pathStr] != currentWriteTime) {
                                fileTimes[pathStr] = currentWriteTime;
                                std::cout << "File changed: " << pathStr << ". Broadcasting reload...\n";
                                
                                // FIX 4: Broadcast safely to all tracked active client connections
                                std::lock_guard<std::mutex> lock(wsClientsMutex);
                                for (auto& client : wsClients) {
                                    client->send("reload");
                                }
                            }
                        } 
                        else {
                            fileTimes[pathStr] = currentWriteTime;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    
    // Block main thread
    httpServer.wait();
    fileWatcher.join(); 
    
    ix::uninitNetSystem();
    return 0;
    
} 








