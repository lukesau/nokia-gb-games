import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import com.nokia.mid.m3d.M3D;
import com.nokia.mid.sound.Sound;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.ChoiceGroup;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Item;
import javax.microedition.lcdui.List;
import javax.microedition.rms.RecordStore;

/**
 * Drives the original Munkiki's Castles classes (KKM, a, b, c, d) tick by
 * tick from a script and logs the game state after every tick.
 *
 * usage: java -cp classes:jardir Harness SCRIPT
 */
public class Harness {
    static KKM kkm;
    static int tick = 0;
    static PrintStream out;
    static int lineNo = 0;

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: Harness SCRIPT");
            System.exit(2);
        }
        out = new PrintStream(new BufferedOutputStream(new FileOutputStream(FileDescriptor.out), 1 << 16), false, "UTF-8");
        System.setOut(out);
        try {
            init();
            BufferedReader rd = new BufferedReader(new FileReader(args[0]));
            String line;
            while ((line = rd.readLine()) != null) {
                lineNo++;
                exec(line.trim());
            }
            rd.close();
        } catch (Throwable t) {
            out.flush();
            System.err.println("HARNESS FAILURE at script line " + lineNo + ", tick " + tick);
            t.printStackTrace();
            out.flush();
            System.exit(1);
        }
        out.flush();
    }

    // ------------------------------------------------------------------
    // Replicates KKM.h() + KKM.init() + startApp's setCurrent, without the Timer.

    static void init() throws Exception {
        RecordStore.clearAll();
        kkm = new KKM();
        // h(): no OPT store -> defaults
        kkm.k = true;
        kkm.j = true;
        kkm.w = 1;

        // init(): language strings
        Class<?> clazz = kkm.getClass();
        InputStream in = null;
        String loc = System.getProperty("microedition.locale");
        if (loc != null) {
            in = clazz.getResourceAsStream("/" + loc);
            if (in == null && loc.length() > 2) {
                in = clazz.getResourceAsStream("/" + loc.substring(0, 2));
            }
        }
        if (in == null) {
            in = clazz.getResourceAsStream("/en");
        }
        kkm.ao = new StringBuffer[32];
        for (int i = 0; i < 32; i++) {
            kkm.ao[i] = new StringBuffer("");
            while (true) {
                int hi = in.read();
                int lo = in.read();
                if (hi + lo <= 0) {
                    break;
                }
                kkm.ao[i].append((char) (hi * 256 + lo));
            }
        }
        in.close();

        // init(): sounds from "/d" offset 0
        in = kkm.f(0);
        kkm.l = new Sound[12];
        for (int i = 0; i < kkm.l.length; i++) {
            int n = in.read();
            n = (short) (n | ((short) in.read() & 0xFF) << 8);
            byte[] data = new byte[n];
            in.read(data);
            kkm.l[i] = new Sound(data, 1);
            kkm.l[i].setGain(100);
        }
        in.close();

        d.ab(kkm);
        c.ab(kkm);
        kkm.am = Display.getDisplay(kkm);
        kkm.al = new b(kkm);
        kkm.p((byte) 0);
        kkm.an = new boolean[12];
        kkm.z = new java.util.Timer(true); // never scheduled: destroyApp cancels it

        // init(): high scores from "MKK" (absent -> n, m stay 0)
        try {
            RecordStore rs = RecordStore.openRecordStore("MKK", false);
            byte[] rec = rs.getRecord(1);
            rs.closeRecordStore();
            kkm.n = ((rec[0] & 0xff) << 8) + (rec[1] & 0xff);
            kkm.m = ((rec[2] & 0xff) << 8) + (rec[3] & 0xff);
        } catch (Exception e) {
            // no high scores yet
        }

        // startApp()
        kkm.am.setCurrent(kkm.al);
    }

    // ------------------------------------------------------------------
    // Script commands

    static void exec(String line) throws Exception {
        if (line.isEmpty() || line.startsWith("#")) {
            return;
        }
        String[] w = line.split("\\s+");
        String cmd = w[0];
        if (cmd.equals("tick")) {
            int n = w.length > 1 ? Integer.parseInt(w[1]) : 1;
            for (int i = 0; i < n; i++) {
                tick++;
                kkm.m();
                logState("");
            }
        } else if (cmd.equals("press")) {
            kkm.al.keyPressed(keyCode(w[1]));
        } else if (cmd.equals("release")) {
            kkm.al.keyReleased(keyCode(w[1]));
        } else if (cmd.equals("key")) {
            int k = keyCode(w[1]);
            kkm.al.keyPressed(k);
            kkm.al.keyReleased(k);
        } else if (cmd.equals("select")) {
            Displayable cur = kkm.am.getCurrent();
            if (!(cur instanceof List)) {
                fail("select: current displayable is not a List: " + describe(cur));
            }
            List list = (List) cur;
            list.setSelectedIndex(Integer.parseInt(w[1]), true);
            kkm.commandAction(List.SELECT_COMMAND, list);
        } else if (cmd.equals("back")) {
            kkm.commandAction(kkm.u, kkm.am.getCurrent());
        } else if (cmd.equals("more")) {
            kkm.commandAction(kkm.s, kkm.am.getCurrent());
        } else if (cmd.equals("choice")) {
            Displayable cur = kkm.am.getCurrent();
            if (!(cur instanceof Form)) {
                fail("choice: current displayable is not a Form: " + describe(cur));
            }
            Form form = (Form) cur;
            int want = Integer.parseInt(w[1]);
            int sel = Integer.parseInt(w[2]);
            int seen = 0;
            ChoiceGroup cg = null;
            for (int i = 0; i < form.size(); i++) {
                Item it = form.get(i);
                if (it instanceof ChoiceGroup) {
                    if (seen == want) {
                        cg = (ChoiceGroup) it;
                        break;
                    }
                    seen++;
                }
            }
            if (cg == null) {
                fail("choice: no ChoiceGroup number " + want + " in " + describe(cur));
            }
            cg.setSelectedIndex(sel, true);
            kkm.itemStateChanged(cg);
        } else if (cmd.equals("dump")) {
            logState(" dump");
        } else if (cmd.equals("frame")) {
            writePgm(w[1]);
        } else if (cmd.equals("quiet")) {
            // state is always logged; accepted for compatibility
        } else {
            fail("unknown command: " + line);
        }
    }

    static void fail(String msg) {
        out.flush();
        throw new RuntimeException(msg);
    }

    static int keyCode(String k) {
        if (k.length() == 1 && Character.isDigit(k.charAt(0))) {
            return 48 + (k.charAt(0) - '0');
        }
        if (k.equals("star")) return 42;
        if (k.equals("hash")) return 35;
        if (k.equals("up")) return -1;
        if (k.equals("down")) return -2;
        if (k.equals("left")) return -3;
        if (k.equals("right")) return -4;
        if (k.equals("fire")) return -5;
        if (k.equals("soft1")) return -6;
        if (k.equals("soft2")) return -7;
        return Integer.parseInt(k);
    }

    static String describe(Displayable cur) {
        if (cur == null) {
            return "null";
        }
        if (cur instanceof Canvas) {
            return "canvas";
        }
        return cur.toString();
    }

    // ------------------------------------------------------------------
    // State log

    static String b(boolean v) {
        return v ? "1" : "0";
    }

    static void logState(String suffix) {
        StringBuilder sb = new StringBuilder();
        sb.append("T ").append(tick).append(suffix).append('\n');
        sb.append("K aj=").append(kkm.aj).append(" ai=").append(kkm.ai).append(" ab=").append(kkm.ab)
          .append(" ad=").append(kkm.ad).append(" ag=").append(kkm.ag).append(" r=").append(kkm.r)
          .append(" o=").append(kkm.o).append(" q=").append(kkm.q).append(" p=").append(kkm.p)
          .append(" n=").append(kkm.n).append(" m=").append(kkm.m).append(" w=").append(kkm.w)
          .append(" af=").append(b(kkm.af)).append(" ac=").append(b(kkm.ac)).append(" ae=").append(b(kkm.ae))
          .append(" k=").append(b(kkm.k)).append(" j=").append(b(kkm.j))
          .append('\n');
        sb.append("L ").append(describe(kkm.am.getCurrent())).append('\n');
        d dd = kkm.ak;
        if (dd == null) {
            sb.append("D null\n");
        } else {
            sb.append("D aj=").append(dd.aj).append(" c_=").append(dd.c_).append(" cz=").append(dd.cz)
              .append(" cy=").append(dd.cy).append(" cx=").append(dd.cx).append(" cn=").append(dd.cn)
              .append(" bh=").append(dd.bh).append(" cp=").append(dd.cp).append(" cq=").append(dd.cq)
              .append(" cr=").append(dd.cr).append(" ct=").append(dd.ct).append(" cu=").append(dd.cu)
              .append(" cv=").append(dd.cv).append(" cw=").append(dd.cw).append(" co=").append(dd.co)
              .append(" c2=").append(dd.c2).append(" c3=").append(dd.c3).append(" c4=").append(dd.c4)
              .append(" c5=").append(dd.c5).append(" c6=").append(dd.c6).append(" c7=").append(dd.c7)
              .append(" c8=").append(dd.c8).append(" c9=").append(dd.c9).append(" b4=").append(dd.b4)
              .append(" b5=").append(dd.b5).append(" b6=").append(b(dd.b6)).append(" b7=").append(dd.b7)
              .append(" b8=").append(dd.b8).append(" b9=").append(dd.b9).append(" by=").append(dd.by)
              .append(" bx=").append(b(dd.bx)).append(" da=").append(dd.da).append(" db=").append(b(dd.db))
              .append(" cj=").append(dd.cj).append(" ci=").append(dd.ci).append(" ch=").append(dd.ch)
              .append(" cf=").append(dd.cf).append(" ce=").append(dd.ce).append(" cd=").append(dd.cd)
              .append(" cc=").append(dd.cc)
              .append(" c0=").append(dd.c0).append(" c1=").append(dd.c1)
              .append('\n');
        }
        c cc = kkm.al.b;
        sb.append("C bt=").append(cc.bt).append(" bs=").append(b(cc.bs)).append(" br=").append(b(cc.br))
          .append(" bq=").append(cc.bq).append(" bp=").append(cc.bp).append(" bo=").append(cc.bo)
          .append(" bh=").append(cc.bh).append(" bg=").append(cc.bg).append(" bf=").append(cc.bf)
          .append(" be=").append(cc.be).append(" bc=").append(cc.bc).append(" a7=").append(cc.a7)
          .append(" a6=").append(cc.a6).append(" a5=").append(cc.a5).append(" a4=").append(cc.a4)
          .append(" bv=").append(b(cc.bv)).append('\n');
        sb.append("BZ");
        if (dd != null && dd.bz != null) {
            appendInts(sb, dd.bz, dd.by * 8);
        }
        sb.append('\n');
        sb.append("BD");
        if (cc.bd != null) {
            appendInts(sb, cc.bd, cc.be * 8);
        }
        sb.append('\n');
        sb.append("BA");
        if (cc.ba != null) {
            appendInts(sb, cc.ba, cc.bc * 4);
        }
        sb.append('\n');
        sb.append("CL ");
        if (d.cl != null) {
            for (int i = 0; i < d.cl.length; i++) {
                hex(sb, d.cl[i] & 0xff, 2);
            }
        }
        sb.append('\n');
        sb.append("CK ");
        if (d.ck != null) {
            for (int i = 0; i < d.ck.length; i++) {
                hex(sb, d.ck[i] & 0xffff, 4);
            }
        }
        sb.append('\n');
        sb.append("S");
        for (int s : Sound.drainPlayed()) {
            sb.append(' ').append(s);
        }
        sb.append('\n');
        sb.append("M3D\n");
        for (String l : M3D.drainLog()) {
            sb.append("  ").append(l).append('\n');
        }
        sb.append("END\n");
        sb.append("AN");
        for (int i = 0; i < 12; i++) {
            sb.append(' ').append(b(kkm.an[i]));
        }
        sb.append('\n');
        sb.append("KD");
        for (int i = 0; i < 12; i++) {
            sb.append(' ').append(b(kkm.al.d[i]));
        }
        sb.append('\n');
        Graphics g = kkm.al.getCanvasGraphics();
        sb.append("STR ").append(String.join(";", g.stringLog)).append('\n');
        g.stringLog.clear();
        // strings drawn on other images (the text images built by b.b()), prefixed by image serial
        ArrayList<String> others = new ArrayList<String>();
        String canvasPrefix = kkm.al.getBuffer().serial + ":";
        for (String s : Graphics.allStringLog) {
            if (!s.startsWith(canvasPrefix)) {
                others.add(s);
            }
        }
        Graphics.allStringLog.clear();
        sb.append("STRIMG ").append(String.join(";", others)).append('\n');
        out.print(sb);
    }

    static void appendInts(StringBuilder sb, int[] a, int n) {
        n = Math.min(n, a.length);
        for (int i = 0; i < n; i++) {
            sb.append(' ').append(a[i]);
        }
    }

    static final char[] HEX = "0123456789abcdef".toCharArray();

    static void hex(StringBuilder sb, int v, int digits) {
        for (int i = digits - 1; i >= 0; i--) {
            sb.append(HEX[(v >> (i * 4)) & 15]);
        }
    }

    static void writePgm(String file) throws Exception {
        Image img = kkm.al.getBuffer();
        int w = img.getWidth(), h = img.getHeight();
        OutputStream os = new BufferedOutputStream(new FileOutputStream(file));
        os.write(("P5\n" + w + " " + h + "\n255\n").getBytes("US-ASCII"));
        byte[] r = img.getRaster();
        for (int i = 0; i < w * h; i++) {
            os.write(r[i] != 0 ? 0 : 255);
        }
        os.close();
    }
}
