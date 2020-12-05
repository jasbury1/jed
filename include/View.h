#ifndef VIEW_H
#define VIEW_H

class View {
public:
    //NVI idiom style draw method should call overridden drawView() method
    void draw() {
        drawView();
    }

    virtual ~View(){};

private:
    virtual void drawView() = 0;
};

#endif // VIEW_H