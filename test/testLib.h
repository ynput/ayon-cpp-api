
#include <iostream>
#include <string>

// Define a macro to create a CodeBlock object with a custom name
#define with_func(name) for (CodeBlock codeBlock(name); !codeBlock.isDone(); codeBlock.setDone())

class CodeBlock {
    public:
        CodeBlock(const std::string &blockName): blockName(blockName), done(false) {
            std::cout << "Start of " << blockName << std::endl;
        }

        ~CodeBlock() {
            if (!done) {
                std::cout << "End of " << blockName << std::endl;
            }
        }

        bool
        isDone() const {
            return done;
        }

        void
        setDone() {
            done = true;
        }

    private:
        std::string blockName;
        bool done;
};
