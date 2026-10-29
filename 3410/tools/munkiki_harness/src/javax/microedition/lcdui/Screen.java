package javax.microedition.lcdui;

public abstract class Screen extends Displayable {
    private String title;

    Screen(String title) {
        this.title = title;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String s) {
        title = s;
    }
}
