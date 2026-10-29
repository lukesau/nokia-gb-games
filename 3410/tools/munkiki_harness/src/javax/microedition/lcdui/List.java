package javax.microedition.lcdui;

import java.util.ArrayList;

public class List extends Screen {
    public static final int EXCLUSIVE = 1;
    public static final int MULTIPLE = 2;
    public static final int IMPLICIT = 3;

    public static final Command SELECT_COMMAND = new Command("", 1, 0);

    private final int type;
    private final ArrayList<String> strings = new ArrayList<String>();
    private int selectedIndex = -1;

    public List(String title, int type) {
        super(title);
        this.type = type;
    }

    public int append(String s, Image img) {
        strings.add(s);
        if (selectedIndex < 0 && type != MULTIPLE) {
            selectedIndex = 0;
        }
        return strings.size() - 1;
    }

    public int size() {
        return strings.size();
    }

    public String getString(int i) {
        return strings.get(i);
    }

    public int getSelectedIndex() {
        return selectedIndex;
    }

    public void setSelectedIndex(int i, boolean sel) {
        if (sel) {
            selectedIndex = i;
        }
    }

    public boolean isSelected(int i) {
        return i == selectedIndex;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("List \"").append(getTitle()).append("\" [");
        for (int i = 0; i < strings.size(); i++) {
            if (i > 0) {
                sb.append('|');
            }
            sb.append(strings.get(i));
        }
        sb.append("] sel=").append(selectedIndex);
        return sb.toString();
    }
}
