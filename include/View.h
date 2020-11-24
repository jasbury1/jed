#ifndef VIEW_H
#define VIEW_H

class View {
public:
    void draw() {
        drawView();
    }

    virtual ~View(){};

private:
    virtual void drawView() = 0;
};

#endif // VIEW_H