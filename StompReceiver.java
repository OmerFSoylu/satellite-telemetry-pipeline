import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class StompReceiver {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 61613;
    private static final String USER = "omer";
    private static final String PASS = "omer";
    private static final String DEST = "/queue/telemetry";

    private static void sendFrame(OutputStream out, String frame) throws IOException {
        byte[] data = frame.getBytes(StandardCharsets.UTF_8);
        int offset = 0;
        while (offset < data.length) {
            int len = Math.min(4096, data.length - offset);
            out.write(data, offset, len);
            offset += len;
        }
        out.flush();
    }

    private static String readFrame(InputStream in) throws IOException {
        StringBuilder sb = new StringBuilder();
        int b;
        while ((b = in.read()) != -1) {
            if (b == 0) {
                break;
            }
            sb.append((char) b);
        }
        if (b == -1 && sb.length() == 0) {
            return null;
        }
        return sb.toString();
    }

    private static String buildConnectFrame() {
        return "CONNECT\n" +
               "accept-version:1.2\n" +
               "host:" + HOST + "\n" +
               "login:" + USER + "\n" +
               "passcode:" + PASS + "\n" +
               "heart-beat:0,0\n" +
               "\n" +
               "\0";
    }

    private static String buildSubscribeFrame() {
        return "SUBSCRIBE\n" +
               "destination:" + DEST + "\n" +
               "id:0\n" +
               "ack:auto\n" +
               "\n" +
               "\0";
    }

    private static String buildDisconnectFrame() {
        return "DISCONNECT\n\n\0";
    }

    public static void main(String[] args) throws Exception {
        try (Socket socket = new Socket(HOST, PORT)) {
            InputStream in = new BufferedInputStream(socket.getInputStream());
            OutputStream out = socket.getOutputStream();

            sendFrame(out, buildConnectFrame());
            String response = readFrame(in);
            if (response == null || !response.startsWith("CONNECTED")) {
                throw new RuntimeException("STOMP CONNECT failed: " + response);
            }

            sendFrame(out, buildSubscribeFrame());

            Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                try {
                    sendFrame(out, buildDisconnectFrame());
                } catch (IOException ignored) {
                }
            }));

            while (true) {
                String frame = readFrame(in);
                if (frame == null) {
                    break;
                }

                int bodyIndex = frame.indexOf("\n\n");
                String body = bodyIndex >= 0 ? frame.substring(bodyIndex + 2) : "";
                System.out.println(body);
            }
        }
    }
}
