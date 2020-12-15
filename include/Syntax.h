#ifndef SYNTAX_H
#define SYNTAX_H

#include <Model.h>
#include <string>

class Syntax
{
public:
    enum highlightMode
    {
        HL_NORMAL = 0,
        HL_COMMENT,
        HL_MLCOMMENT,
        HL_KEYWORD1,
        HL_KEYWORD2,
        HL_STRING,
        HL_NUMBER,
        HL_MATCH
    };

    Syntax(){}
    virtual ~Syntax(){}

    static std::unique_ptr<Syntax> getSyntax(std::string extension);

    virtual std::string fileType() const = 0;
    virtual void updateSyntaxHighlight(std::vector<Model::erow> &rowList, int rowIndex) const = 0;
    virtual bool highlightStrings() const = 0;
    virtual bool highlightNums() const = 0;
    
};


#endif //SYNTAX_H