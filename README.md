Creates a C++/IxWebSocket command line server application code function classes for rendering HTML documents parsed from text files with delimiters for paragraphs, diagrams, simple graphs and mathematical formulas. 

The text file(s) uploaded in nested directories on the running server contain delimiters showing where the parser/renderer would insert text paragraphs to the current page up to the next delimiters to insert diagram from photo files located in the same directory, or to parse the next block of text as LATEX math formula format up to the next delimiters to read the next block in text file as graph plot data table rendered with simple plotting library. 

All the HTML webpage elements are strictly rendered server-side and the webpage is served on port 9002 to browser with the option to download by highlighting elements being disabled. 

Refreshing the browser webpage will reparse and reload the server-side text file(s) with changes/modifications. 

Implements an automatic file-watcher notification (using platform-specific file events or polling) that auto-refreshes the browser client-side via WebSocket whenever a text file is saved on disk. 

This server implementation includes dynamic chart rendering for SVG-based line charts generated natively with HTML canvas on the backend alongside raw data tables.

The C++/IxWebSocket server code for the renderer fetches the HTML webpage template index.html from file with the respective .css and .js files alongside the source text file strictly for server-side rendering. 

Implements a basic corporate style webpage template in light color themes for displaying documents for the source text file as HTML with delimiters for document title, document headers/subheaders, and text captions in small font below photos, diagrams and graphs, and text explaining variables below each formula. 

Build and Run Instructions: 

Create a build directory 

mkdir build && cd build

Generate build files with CMake

cmake ..

Compile the application

cmake --build . --config Release

Create the document root directory and run the server

cd ../
mkdir -p documents
cp -a source directory/sourcetextfilename.txt documents/ 
cp build/SecureDocServer ./
./SecureDocServer

