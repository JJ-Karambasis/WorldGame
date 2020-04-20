# WorldGame

### How to build and run the game (Windows)
1. Please install git if you have not already: https://git-scm.com/book/en/v2/Getting-Started-Installing-Git
2. You will also need git lfs (large file storage) for the game assets which you can install here: https://git-lfs.github.com/
3. You will also need the microsoft visual studio c++ compiler in order to build the game code. Please install Microsoft Visual Studio Community edition and make sure you include  the c++ development tools: https://visualstudio.microsoft.com/downloads/
4. Open a windows command prompt (Press Alt-R and type cmd and press ok)
5. Change directory into the folder you want the repository to live in
6. Type `git clone https://github.com/JJ-Karambasis/WorldGame.git` which will clone the world game repository into a folder called WorldGame
7. You will need to run a vcvarsall.bat file. The path may vary depending on the version of the compiler you install. For 2019 `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64` should work. If it doesn't, google where the vcvarsall.bat file for the c++ compiler is. If you did not install the c++ development tools in step 3, you will not be able to find this file.
8. Go to the WorldGame root folder. In the WorldGame/Code directory, run the build.bat file. This will build the executable
9. Go to the WorldGame root folder. In the WorldGame/Bin directory, run the Start.bat file. This will run the game and set your working directory to the right path.
