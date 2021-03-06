JED (James EDitor)

A (heavily) WIP OO text editor in C++.

![Screenshot Image](/images/screenshot.png)
Format: ![Alt Text](url)


To Run:
Run make inside the repo start directory
builds a 'jed' executable inside /build/apps
run with no arguments or one argument representing a file name
ex: "./jed test.cpp"

Originally a spin off of a spin off of the Kilo text editor ported to c++:
https://viewsourcecodmodel->org/snaptoken/kilo/index.html

JED promotes modularity and is loosely structured around the Model/View-Controller paradigm.

The Model class is independent of any controllers or views. It is structured around a vector
of row structures representing the rows in a file. Model includes API calls for basic model
interactions that are required by text editors. These include adding new rows, deleting rows,
inserting chars, deleting chars, inserting newlines, and more.
Each instance of Model should be at most one "file" that is open in the editor

Each View should have a corresponding controller.
