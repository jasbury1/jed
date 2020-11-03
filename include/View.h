#ifndef VIEW_H
#define VIEW_H

class View {
public:
    virtual void draw() = 0;
    virtual int getWindowSize(int *rows, int *cols) = 0;
};

#endif /* VIEW_H */