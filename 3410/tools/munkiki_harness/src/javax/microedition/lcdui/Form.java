package javax.microedition.lcdui;

import java.util.ArrayList;

public class Form extends Screen {
    private final ArrayList<Item> items = new ArrayList<Item>();
    private ItemStateListener itemListener;

    public Form(String title) {
        super(title);
    }

    public int append(String s) {
        items.add(new StringItem(null, s));
        return items.size() - 1;
    }

    public int append(Item item) {
        items.add(item);
        return items.size() - 1;
    }

    public Item get(int i) {
        return items.get(i);
    }

    public int size() {
        return items.size();
    }

    public void setItemStateListener(ItemStateListener l) {
        itemListener = l;
    }

    public ItemStateListener getItemStateListener() {
        return itemListener;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Form \"").append(getTitle()).append("\" [");
        for (int i = 0; i < items.size(); i++) {
            if (i > 0) {
                sb.append("; ");
            }
            sb.append(items.get(i).toString());
        }
        sb.append(']');
        return sb.toString();
    }
}
