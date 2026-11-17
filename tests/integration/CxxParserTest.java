import java.nio.file.Files;
import java.nio.file.Path;
import com.sonar.cxx.sslr.api.RecognitionException;
import org.sonar.cxx.parser.CxxParser;
import org.sonar.cxx.config.CxxSquidConfiguration;
import org.sonar.cxx.squidbridge.SquidAstVisitorContextImpl;
import org.sonar.cxx.squidbridge.api.SourceProject;
import org.sonar.cxx.api.CxxMetric;

// Real installed grammar, strict mode. Full include context is checked by Sonar.
public class CxxParserTest {
    private static void parse(Path file, String source) {
        var config = new CxxSquidConfiguration();
        config.add(CxxSquidConfiguration.SONAR_PROJECT_PROPERTIES,
                   CxxSquidConfiguration.ERROR_RECOVERY_ENABLED, "false");
        var context = new SquidAstVisitorContextImpl<>(new SourceProject("", null));
        context.setFile(file.toFile(), CxxMetric.FILES);
        CxxParser.create(context, config).parse(source);
    }

    private static void reject(Path file, String source) {
        try { parse(file, source); }
        catch (RecognitionException expected) { return; }
        throw new AssertionError("Negative control unexpectedly parsed: " + file);
    }

    public static void main(String[] args) throws Exception {
        Path header = Path.of(args[0], "extension/core/swdebuglog.h");
        // Isolate grammar from includes without changing scanner inputs.
        String source = Files.readString(header).replaceAll("(?m)^#.*$", "");
        parse(header, source);
        String oldDefault = source.replace("size_t limit = (1024 * 1024)", "size_t limit = 1024 * 1024");
        if (source.equals(oldDefault)) throw new AssertionError("Missing default argument control");
        reject(header, oldDefault);

        Path sdk = Path.of(args[0], "extension/sdk/smsdk_ext.cpp");
        var lines = Files.readAllLines(sdk);
        String export = lines.stream().filter(s -> s.startsWith("#define SMEXT_DLL_EXPORT") && s.contains("gnu::visibility"))
            .findFirst().orElseThrow().replaceFirst("#define\\s+SMEXT_DLL_EXPORT\\s+", "");
        var signatures = lines.stream().filter(s -> s.startsWith("SMEXT_DLL_EXPORT ")).toList();
        if (signatures.size() != 2) throw new AssertionError("Expected both production factory exports");
        for (String signature : signatures) {
            String definition = signature.replace("SMEXT_DLL_EXPORT", export) + " { return nullptr; }";
            parse(sdk, definition);
            reject(sdk, definition.replace("[[gnu::visibility(\"default\")]]", "__attribute__((visibility(\"default\")))"));
        }
        reject(sdk, "int broken( {");
        System.out.println("PASS: CXX strict syntax; DebugLog and both factories, four negative controls");
    }
}
