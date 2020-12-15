#include <string>

#include "Syntax.h"
#include "CSyntax.h"

std::unique_ptr<Syntax> Syntax::getSyntax(std::string extension)
{
    if (extension.compare("c") == 0 ||
        extension.compare("h") == 0)
    {
        return std::make_unique<CSyntax>();
    }
    return std::make_unique<CSyntax>();
}
