# nmResource
Library for embedding resources in C/C++ projects


## Usage

### Program Arguments
- ```-r, --recursive```: embeds all resources in current directory and sub directories (default=OFF)
- ```--cwd 'dir/'```: sets root directory for resource search to dir/ (default="./")
- ```--namespace 'name'```: sets default namespace of embedded resources (default="Res")
- ```--suffix '.ext'```: sets default resource suffix (default=".h")
- ```--rules 'file.txt'```: sets default rules file (default="./res_rules.txt")

### File rules
- Write resource suffix to ```res_rules.txt``` in current working directory (separated by newline)
- Any line that doesn't start with '>' will embed files with given suffix ```(.png, res.png ...)```
- Lines start with '>' will be splitted by '*', first part being prefix and second part being suffix. ```(>resources/*.png)```


## Building
- Make sure you have cmake installed in your system
```
mkdir build
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/
./build/nmres ...
```