package javax.microedition.lcdui;

import java.util.ArrayList;

public abstract class Displayable {
    private final ArrayList<Command> commands = new ArrayList<Command>();
    private CommandListener listener;

    Displayable() {
    }

    public void addCommand(Command cmd) {
        if (!commands.contains(cmd)) {
            commands.add(cmd);
        }
    }

    public void removeCommand(Command cmd) {
        commands.remove(cmd);
    }

    public void setCommandListener(CommandListener l) {
        listener = l;
    }

    public CommandListener getCommandListener() {
        return listener;
    }

    public java.util.List<Command> getCommands() {
        return commands;
    }

    public int getWidth() {
        return 96;
    }

    public int getHeight() {
        return 65;
    }

    public boolean isShown() {
        return Display.isCurrent(this);
    }
}
