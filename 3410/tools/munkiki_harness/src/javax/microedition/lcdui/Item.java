package javax.microedition.lcdui;

public abstract class Item {
    private String label;

    Item(String label) {
        this.label = label;
    }

    public String getLabel() {
        return label;
    }

    public void setLabel(String s) {
        label = s;
    }
}
