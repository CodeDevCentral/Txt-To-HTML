Creates a C++/IxWebSocket command line server application code function classes for rendering HTML documents parsed from text files with delimiters for paragraphs, diagrams, simple graphs and mathematical formulas. 

The text files uploaded in nested directories on the running server should contain delimiters showing where the parser/renderer would insert text paragraphs to the current page up to the next delimiters to insert diagram from photo files in the same directory or to parse the next block of text as LATEX math formula format up to the next delimiters to read the next block in text file as graph plot data table rendered with simple plotting library. 

All the HTML webpage elements should strictly be rendered server-side and served on port 9002 to browser with the option to download by highlighting elements being disabled. 

Ensure that refreshing the browser webpage would reparse and reload the server-side text files with changes/modifications. 

To add an automatic file-watcher notification (using platform-specific file events or polling) that auto-refreshes the browser client-side via WebSocket whenever a text file is saved on disk. 

To extend this server implementation to include dynamic chart rendering such as SVG-based line charts generated natively with HTML canvas on the backend instead of raw data tables.

For the C++/IxWebSocket server code for the renderer, show only code line changes to make the server application fetch the HTML webpage template index.html from file with the respective .css and .js files alongside the source text file strictly for server-side rendering. 

Create a basic corporate style webpage template for software developer company in light color themes for displaying documents for the source text file as HTML with delimiters for document title, document headers/subheaders, and text captions in small font below photos, diagrams and graphs, and text explaining variables below each formula. Show only code lines modified/changes in the C++/IxWebSocket code file. 



# 1. Create a build directory 

mkdir build && cd build

# 2. Generate build files with CMake

cmake ..

# 3. Compile the application

cmake --build . --config Release

# 4. Create the document root directory and run the server

cd ../
mkdir -p documents
cp -a source directory/sourcetextfile.txt documents/ 
cp build/SecureDocServer ./
./SecureDocServer

