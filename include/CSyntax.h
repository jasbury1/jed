#ifndef CSYNTAX_H
#define CSYNTAX_H

#include "Syntax.h"
#include "Model.h"

#include <vector>
#include <string>

class CSyntax : public Syntax
{
public:
    CSyntax();
    ~CSyntax();
    void updateSyntaxHighlight(std::vector<Model::erow>& rowList, std::size_t rowIndex) const;

    bool highlightStrings() const {return true;}
    bool highlightNums() const {return true;}

    std::string fileType() const {return "c";}

private:
    std::vector<std::string> keywords = {
        "switch", "if", "while", "for", "break", "continue", "return", "else",
        "struct", "union", "typedef", "static", "enum", "class", "case"
    };

    std::vector<std::string> types = {
        "int", "long", "double", "float", "char", "unsigned", "signed", "void"
    };

    std::string singlelineComment{"//"};
    std::string multilineCommentStart{"/*"};
    std::string multilineCommentEnd{"*/"};
};

#endif //CSYNTAX_H