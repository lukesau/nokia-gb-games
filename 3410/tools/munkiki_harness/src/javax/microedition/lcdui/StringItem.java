package javax.microedition.lcdui;

/** Used by Form.append(String). */
public class StringItem extends Item {
    private String text;

    public StringItem(String label, String text) {
        super(label);
        this.text = text;
    }

    public String getText() {
        return text;
    }

    public void setText(String s) {
        text = s;
    }

    public String toString() {
        return "\"" + text + "\"";
    }
}
