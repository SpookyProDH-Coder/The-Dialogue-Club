#!/bin/bash
#
#   - The tool for building the chatroom app.
#

#Color palletes
NC="\e[0m"
RED="\e[1;31m"
GREEN="\e[32m"
BLUE="\e[34m"

echo "Checking the following dependencies: "
echo -e "\t- gcc."

checkDependencies() {
    for i in "$@"; do
        dpkg --status $i &>/dev/null
        installed=$?
        if [ $installed -eq 0 ]; then
            echo -e "\t- The package ${i} is already installed."
        else
            echo -e "- The package ${i} is not installed."
            echo -e "\n\tInstalling ${i}..."
            sudo apt install -y ${i} 2>/dev/null
            if [ $? -eq 0 ]; then     # Error handling
                echo -e "${GREEN}\n\t- Successfully installed ${i}.${NC}"
            else
                echo -e "${RED}\n\t- Failed to install ${i}.${NC}"
            fi
        fi
    done
    
    echo -e "\nInstallation completed."
}



function main() {
    dependencies=("gcc"); #dependencies=("gcc" "g++");
    checkDependencies "${dependencies[@]}"

    echo "Compiling main.c"  

    # Creating object files for each .c file
    src_files=("main.c" "./includes/helpers.c" "./includes/encryption.c")
    obj_files=()

    for src in "${src_files[@]}"; do
        obj="${src%.c}.o"
        obj_files+=("$obj")
        echo "Compiling $src into $obj"
        gcc -c $src -o $obj
    done

    echo "Created object files: ${obj_files[@]}"

    # Linking each obj file
    gcc "${obj_files[@]}" -o Chatroom.out -pthread -lm

    # Removing the objects
    rm "${obj_files[@]}"

    echo "Program execution finished."
}

main