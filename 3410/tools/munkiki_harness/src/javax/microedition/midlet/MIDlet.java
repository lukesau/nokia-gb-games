package javax.microedition.midlet;

public abstract class MIDlet {
    protected MIDlet() {
    }

    protected abstract void startApp() throws MIDletStateChangeException;

    protected abstract void pauseApp();

    protected abstract void destroyApp(boolean unconditional) throws MIDletStateChangeException;

    public final void notifyDestroyed() {
    }

    public final void notifyPaused() {
    }

    public final String getAppProperty(String key) {
        return null;
    }
}
