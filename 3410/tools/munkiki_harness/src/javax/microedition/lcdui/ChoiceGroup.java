package javax.microedition.lcdui;

import java.util.ArrayList;

public class ChoiceGroup extends Item {
    public static final int EXCLUSIVE = 1;
    public static final int MULTIPLE = 2;

    private final int type;
    private final ArrayList<String> strings = new ArrayList<String>();
    private final ArrayList<Boolean> selected = new ArrayList<Boolean>();

    public ChoiceGroup(String label, int type) {
        super(label);
        this.type = type;
    }

    public int append(String s, Image img) {
        strings.add(s);
        // EXCLUSIVE: the first element appended becomes selected
        selected.add(type == EXCLUSIVE && strings.size() == 1);
        return strings.size() - 1;
    }

    public int size() {
        return strings.size();
    }

    public String getString(int i) {
        return strings.get(i);
    }

    public boolean isSelected(int i) {
        return selected.get(i);
    }

    public int getSelectedIndex() {
        if (type != EXCLUSIVE) {
            return -1;
        }
        for (int i = 0; i < selected.size(); i++) {
            if (selected.get(i)) {
                return i;
            }
        }
        return -1;
    }

    public void setSelectedIndex(int i, boolean sel) {
        if (type == EXCLUSIVE) {
            if (sel) {
                for (int k = 0; k < selected.size(); k++) {
                    selected.set(k, k == i);
                }
            }
        } else {
            selected.set(i, sel);
        }
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("ChoiceGroup \"").append(getLabel()).append("\" (");
        for (int i = 0; i < strings.size(); i++) {
            if (i > 0) {
                sb.append('|');
            }
            sb.append(strings.get(i));
        }
        sb.append(") sel=").append(getSelectedIndex());
        return sb.toString();
    }
}
