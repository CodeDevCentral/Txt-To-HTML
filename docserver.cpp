#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXNetSystem.h> 
#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex> 
#include <set>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <cctype>
#include <cstdlib>


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
            else if (line.rfind("[TABLE]", 0) == 0) {
                flushBlock();
                currentType = BlockType::Table;
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

class SvgChartRenderer {
private:
    struct Point {
        double x;
        double y;
    };

    struct Series {
        std::string name;
        std::vector<Point> points;
    };

    static constexpr int WIDTH = 900;
    static constexpr int HEIGHT = 480;
    static constexpr double LEFT = 85;
    static constexpr double RIGHT = 40;
    static constexpr double TOP = 55;
    static constexpr double BOTTOM = 85;
    static constexpr double PLOT_WIDTH = WIDTH - LEFT - RIGHT;
    static constexpr double PLOT_HEIGHT = HEIGHT - TOP - BOTTOM;

    static const std::vector<std::string>& colors() {
        static const std::vector<std::string> palette = {
            "#2563eb", "#dc2626", "#16a34a",
            "#9333ea", "#ea580c", "#0891b2",
            "#be185d"
        };
        return palette;
    }

    static std::string trim(const std::string& value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    static std::string escapeXml(const std::string& value) {
        std::string result;
        for (char c : value) {
            switch (c) {
                case '&': result += "&amp;"; break;
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&apos;"; break;
                default: result += c;
            }
        }
        return result;
    }

    static std::vector<std::string> splitCsv(const std::string& line) {
        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(trim(field));
        // Preserve a final empty column.
        if (!line.empty() && line.back() == ',') fields.emplace_back(); 
        return fields;
    }

    static bool parseNumber(const std::string& input, double& result) {
        std::string value = trim(input);
        if (value.empty()) return false;
        char* end = nullptr;
        result = std::strtod(value.c_str(), &end);
        return end != value.c_str() && *end == '\0' && std::isfinite(result);
    }

    static double scaleX(double value, double minX, double maxX) {
        if (maxX == minX) return LEFT + PLOT_WIDTH / 2.0;
        return LEFT + ((value - minX) / (maxX - minX)) * PLOT_WIDTH;
    }

    static double scaleY(double value, double minY, double maxY) {
        if (maxY == minY) return TOP + PLOT_HEIGHT / 2.0;
        return TOP + PLOT_HEIGHT - ((value - minY) / (maxY - minY)) * PLOT_HEIGHT;
    }

    static std::string formatNumber(double value) {
        std::ostringstream output;
        output << std::setprecision(5) << value;
        return output.str();
    }

    static void emit(std::ostringstream& svg, const std::string& element) {
        svg << element << '\n';
    }

public:

    static std::string render(const std::string& csv, const std::string& chartTitle = "Data Chart") {
        std::stringstream input(csv);
        std::string line;
        std::vector<std::string> headers;
        // Read CSV header.
        while (std::getline(input, line)) {
            line = trim(line);
            if (!line.empty()) {
                headers = splitCsv(line);
                break;
            }
        }
        if (headers.size() < 2) {
            return "<div class='chart-error'>"
                   "Invalid graph: at least two CSV columns are required."
                   "</div>";
        }

        const std::size_t columnCount = headers.size();
        bool categoricalSeries = false;
        // Recognize the format X,Y,Series.
        if (columnCount == 3) {
            std::string third = headers[2];
            std::transform(third.begin(), third.end(), third.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                }
            ); 
            categoricalSeries = (third == "series");
        } 
        
        std::vector<Series> series;
        std::map<std::string, std::size_t> seriesIndex;
        if (!categoricalSeries) {
            // Standard numeric-column format:
            // X,Series1,Series2,...
            for (std::size_t i = 1; i < columnCount; ++i) series.push_back({headers[i], {}}); 
        }

        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();
        std::size_t validRows = 0;
        std::size_t invalidRows = 0;
        while (std::getline(input, line)) {
            line = trim(line);
            if (line.empty()) continue; 
            
            const auto fields = splitCsv(line);
            if (fields.size() != columnCount) {
                ++invalidRows;
                continue;
            }
            double x;
            if (!parseNumber(fields[0], x)) {
                ++invalidRows;
                continue;
            }
            if (categoricalSeries) {
                // Format: X,Y,Series
                double y;
                if (!parseNumber(fields[1], y) || fields[2].empty()) {
                    ++invalidRows;
                    continue;
                }
                
                const std::string name = fields[2];
                auto it = seriesIndex.find(name);
                if (it == seriesIndex.end()) {
                    const std::size_t index = series.size();
                    seriesIndex[name] = index;
                    series.push_back({name, {}});
                }

                Series& current =  series[seriesIndex[name]];
                current.points.push_back({x, y});

                minY = std::min(minY, y);
                maxY = std::max(maxY, y);

            } 
            
            else {
                // Format: X,Y1,Y2,Y3,...
                std::vector<double> values;
                bool valid = true;
                for (std::size_t i = 1; i < columnCount; ++i) {
                    double y;
                    if (!parseNumber(fields[i], y)) {
                        valid = false;
                        break;
                    }
                    values.push_back(y);
                }

                if (!valid) {
                    ++invalidRows;
                    continue;
                }

                for (std::size_t i = 0; i < values.size(); ++i) {
                    series[i].points.push_back({x, values[i]});
                    minY = std::min(minY, values[i]);
                    maxY = std::max(maxY, values[i]);
                }
            }

            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            ++validRows;
        }

        // Reject empty or invalid datasets.
        if (validRows == 0 || series.empty() || minX == std::numeric_limits<double>::max() || minY == std::numeric_limits<double>::max()) {
            return "<div class='chart-error'>"
                   "No valid numeric graph data was found."
                   "</div>";
        }

        // Expand constant ranges to prevent division by zero.
        if (minX == maxX) {
            minX -= 1.0;
            maxX += 1.0;
        }

        if (minY == maxY) {
            const double padding = std::max(1.0, std::abs(minY) * 0.1);
            minY -= padding;
            maxY += padding;
        } 
        
        else {
            const double padding = (maxY - minY) * 0.08;
            minY -= padding;
            maxY += padding;
        }

        std::ostringstream svg;
        svg << std::fixed << std::setprecision(2);
        emit(svg, "<div class='graph-block'>");
        emit(svg,
            "<svg class='corp-chart' "
            "xmlns='http://www.w3.org/2000/svg' "
            "viewBox='0 0 900 480' "
            "role='img' "
            "aria-label='" + escapeXml(chartTitle) + "'>");

        // Background.
        emit(svg,
            "<rect width='900' height='480' "
            "fill='#ffffff' rx='8'/>");

        // Chart title.
        emit(svg,
            "<text x='450' y='30' "
            "text-anchor='middle' "
            "font-size='19' font-weight='600' "
            "fill='#1e293b'>" +
            escapeXml(chartTitle) +
            "</text>");

        // Gridlines and Y-axis labels.
        constexpr int GRID_LINES = 6;
        for (int i = 0; i <= GRID_LINES; ++i) {
            const double fraction = static_cast<double>(i) / GRID_LINES;
            const double y = TOP + fraction * PLOT_HEIGHT;
            const double value = maxY - fraction * (maxY - minY);

            emit(svg,
                "<line x1='85' y1='" +
                formatNumber(y) +
                "' x2='860' y2='" +
                formatNumber(y) +
                "' stroke='#e2e8f0' stroke-width='1'/>");

            emit(svg,
                "<text x='72' y='" +
                formatNumber(y + 4) +
                "' text-anchor='end' "
                "font-size='11' fill='#64748b'>" +
                formatNumber(value) +
                "</text>");
        }

        // Axes.
        emit(svg,
            "<line x1='85' y1='55' x2='85' y2='395' "
            "stroke='#475569' stroke-width='1.5'/>");

        emit(svg,
            "<line x1='85' y1='395' x2='860' y2='395' "
            "stroke='#475569' stroke-width='1.5'/>");

        // X-axis tick labels.
        for (int i = 0; i <= GRID_LINES; ++i) {
            const double fraction = static_cast<double>(i) / GRID_LINES;
            const double x = LEFT + fraction * PLOT_WIDTH;
            const double value = minX + fraction * (maxX - minX);

            emit(svg,
                "<text x='" + formatNumber(x) +
                "' y='415' text-anchor='middle' "
                "font-size='11' fill='#64748b'>" +
                formatNumber(value) +
                "</text>");
        }

        // Axis titles.
        emit(svg,
            "<text x='472' y='455' "
            "text-anchor='middle' "
            "font-size='13' fill='#334155'>"
            + escapeXml(headers[0]) +
            "</text>");

        emit(svg,
            "<text transform='translate(20 225) rotate(-90)' "
            "text-anchor='middle' "
            "font-size='13' fill='#334155'>" +
            escapeXml(categoricalSeries
                ? headers[1]
                : "Value") +
            "</text>");

        // Draw each data series.
        const auto& palette = colors();
        for (std::size_t s = 0; s < series.size(); ++s) {
            const auto& current = series[s];
            if (current.points.empty()) continue;

            std::ostringstream points;
            for (const auto& point : current.points) {
                points << formatNumber(
                    scaleX(point.x, minX, maxX)
                ) << ","
                << formatNumber(
                    scaleY(point.y, minY, maxY)
                ) << " ";
            }

            const std::string color = palette[s % palette.size()];

            emit(svg,
                "<polyline points='" +
                points.str() +
                "' fill='none' stroke='" +
                color +
                "' stroke-width='2.5' "
                "stroke-linejoin='round' "
                "stroke-linecap='round'/>");

            // Data points.
            for (const auto& point : current.points) {
                emit(svg,
                    "<circle cx='" +
                    formatNumber(
                        scaleX(point.x, minX, maxX)
                    ) +
                    "' cy='" +
                    formatNumber(
                        scaleY(point.y, minY, maxY)
                    ) +
                    "' r='4' fill='" +
                    color +
                    "' stroke='white' stroke-width='1.5'/>");
            }

            // Legend.
            const double legendX = 110 + s * 155;

            emit(svg,
                "<line x1='" +
                formatNumber(legendX) +
                "' y1='440' x2='" +
                formatNumber(legendX + 22) +
                "' y2='440' stroke='" +
                color +
                "' stroke-width='3'/>");

            emit(svg,
                "<text x='" +
                formatNumber(legendX + 28) +
                "' y='444' font-size='11' "
                "fill='#334155'>" +
                escapeXml(current.name) +
                "</text>");
        }

        emit(svg, "</svg>");

        if (invalidRows > 0) {
            emit(svg,
                "<p class='chart-warning'>"
                + std::to_string(invalidRows) +
                " invalid data row(s) skipped.</p>");
        }

        emit(svg, "</div>");

        return svg.str();
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
                case BlockType::Table: {
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
                case BlockType::Graph: {
                    const std::string title = block.metadata.empty() ? "Data Chart" : block.metadata;
                    contentStream << SvgChartRenderer::render(block.content, title); 
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








