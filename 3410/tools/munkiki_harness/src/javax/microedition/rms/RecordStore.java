package javax.microedition.rms;

import java.util.ArrayList;
import java.util.LinkedHashMap;

/** In-memory record stores keyed by name. Record ids start at 1. */
public class RecordStore {
    private static final LinkedHashMap<String, ArrayList<byte[]>> stores = new LinkedHashMap<String, ArrayList<byte[]>>();

    private final String name;
    private final ArrayList<byte[]> records;

    private RecordStore(String name, ArrayList<byte[]> records) {
        this.name = name;
        this.records = records;
    }

    public static RecordStore openRecordStore(String name, boolean createIfNecessary) throws RecordStoreException {
        ArrayList<byte[]> recs = stores.get(name);
        if (recs == null) {
            if (!createIfNecessary) {
                throw new RecordStoreNotFoundException(name);
            }
            recs = new ArrayList<byte[]>();
            stores.put(name, recs);
        }
        return new RecordStore(name, recs);
    }

    public static void deleteRecordStore(String name) throws RecordStoreException {
        if (stores.remove(name) == null) {
            throw new RecordStoreNotFoundException(name);
        }
    }

    public static String[] listRecordStores() {
        return stores.keySet().toArray(new String[0]);
    }

    /** Harness: forget every store. */
    public static void clearAll() {
        stores.clear();
    }

    public void closeRecordStore() {
    }

    public String getName() {
        return name;
    }

    public int addRecord(byte[] data, int off, int len) throws RecordStoreException {
        byte[] copy = new byte[len];
        if (len > 0) {
            System.arraycopy(data, off, copy, 0, len);
        }
        records.add(copy);
        return records.size();
    }

    public byte[] getRecord(int id) throws RecordStoreException {
        if (id < 1 || id > records.size()) {
            throw new InvalidRecordIDException(name + "#" + id);
        }
        return records.get(id - 1).clone();
    }

    public int getRecord(int id, byte[] buf, int off) throws RecordStoreException {
        byte[] r = getRecord(id);
        System.arraycopy(r, 0, buf, off, r.length);
        return r.length;
    }

    public int getRecordSize(int id) throws RecordStoreException {
        return getRecord(id).length;
    }

    public int getNumRecords() {
        return records.size();
    }

    public int getNextRecordID() {
        return records.size() + 1;
    }

    public void setRecord(int id, byte[] data, int off, int len) throws RecordStoreException {
        if (id < 1 || id > records.size()) {
            throw new InvalidRecordIDException(name + "#" + id);
        }
        byte[] copy = new byte[len];
        System.arraycopy(data, off, copy, 0, len);
        records.set(id - 1, copy);
    }
}
