# ai_command_line
An interactive C++ script that leverages OpenAI GPT-4 for human language interaction with Linux command line.

This project demonstrates the potential of using OpenAI's GPT-4 as a command line interpreter to convert human language queries into operating system-specific command line instructions. It serves as a proof of concept and a fun tool to explore the possibilities of AI-powered command line interfaces.

## Prerequisites
To use this script, you will need an OpenAI API key with access to GPT-4. GPT-3, while powerful, is not optimized for the type of output required in this application.

## Features
Human language interpretation for Linux command line tasks
Context-aware AI assistant that remembers the conversation
Basic logging functionality to track user inputs, AI responses, and command outputs

## Getting Started
Clone this repository to your local machine
Install the required libraries: libcurl, nlohmann-json, and libssl-dev
Compile the script using a C++ compiler that supports C++14 or higher (e.g., g++ -std=c++14 ai_command_line.cpp -o command_line_simulator -lcurl)
Run the compiled executable (e.g., ./command_line_simulator)

## Usage
Upon running the script, you will be prompted to enter your OpenAI API key if it's not already stored in the config file. After that, the script will present you with a simulated Linux command line environment where you can type commands in human language.

The AI assistant will process your input and respond with the appropriate Linux command(s) to execute. The script will then execute the command(s) and display the output. You can also enable or disable logging during runtime by typing 'log on' or 'log off', respectively.

Note: This project is intended as a proof of concept and should not be used in production or on critical systems. While the AI command line interpreter works well for many tasks, it may not be perfect and could potentially execute undesired commands. Use at your own risk.

## License
This project is licensed under the MIT License.
