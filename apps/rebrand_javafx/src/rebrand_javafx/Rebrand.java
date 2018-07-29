/*
 * Software
 * Copyright  2018 Gregery Barton
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it freely.
 *
 */
package rebrand_javafx;

import java.io.Console;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.AbstractMap.SimpleEntry;
import java.util.Collection;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;
import org.apache.commons.io.FileUtils;
import org.apache.commons.io.FilenameUtils;
import org.apache.commons.io.filefilter.IOFileFilter;
import org.apache.commons.lang3.StringUtils;
import org.apache.commons.lang3.tuple.ImmutablePair;
import org.apache.commons.lang3.tuple.ImmutableTriple;

/**
 *
 * @author Gregery Barton
 */
public class Rebrand {

 private static final IOFileFilter FILTER_FILES = new IOFileFilter() {
  @Override
  public boolean accept(File file) {
   String name = file.getName().toLowerCase();
   String extension = FilenameUtils.getExtension(name);
   if (name.equals("rebrand_javafx")) {
    return false;
   } else if (StringUtils.containsIgnoreCase(name, "LICENSE")) {
    return false;
   }
   switch (extension.toLowerCase()) {
    case "png":
    case "gif":
    case "jpg":
    case "dll":
    case "jar":
    case "zip":
    case "7z":
    case "gzip":
    case "tar":
     return false;
    default:
     if (file.isDirectory()) {
      switch (name) {
       case ".git":
       case ".github":
       case "build":
        return false;
       default:
        return true;
      }
     }
     return true;
   }
  }

  @Override
  public boolean accept(File dir, String name) {
   throw new AssertionError();
  }

 };

 /**
  * @param args the command line arguments
  */
 public static void main(String[] args) {
  if (args.length < 4) {
   printUsage();
   System.exit(1);
  }
  File path = new File(args[0]);
  String javafx = args[1];
  String com_sun = args[2];
  String com_oracle = args[3];
  boolean reverse = false;
  if (args.length >= 5) {
   reverse = args[4].equalsIgnoreCase("reverse");
   if (!reverse) {
    printUsage();
    System.exit(1);
   }
  }
  if (!reverse) {
   System.out.println("Change javafx\t->\t" + javafx);
   System.out.println("Change sun\t->\t" + com_sun);
   System.out.println("Change oracle\t->\t" + com_oracle);
   System.out.println("\n\n");
  } else {
   System.out.println("Change " + javafx + "\t->\tjavafx");
   System.out.println("Change " + com_sun + "\t->\tsun");
   System.out.println("Change " + com_oracle + "\t->\toracle");
   System.out.println("\n");
  }
  System.out.println(""
   + "\n- After the operation is complete all projects must be completely rebuilt including all native code. "
   + "\n- A default gradle build is not enough."
   + "\n- Refactoring does not work if a target identifier is split over multiple lines."
   + "\n- Repositories other than those in the default GitHub folders will be corrupted."
   + "\n- Binary files introduced into the folder tree after initial clone from GitHub may also be corrupted."
   + "\n- License text may be altered if the license text contains the literals 'javafx', 'com.sun', or 'com.oracle',"
   + "\n  however the standard license from these companies don't contain JavaFX specific text. "
   + "\n- It's your responsibility to make sure any other 'Must not be altered' text is unchanged or restored "
   + "\n  to its unaltered form."
   + "\n- Search and replace ignores case."
   + "\n- The process is not perfect and will require some manual corrections. Make sure you have a BACKUP!\n");
  Console console = System.console();
  if (console != null) {
   int wake_test = new Random().nextInt();
   System.err.println("Type the number to confirm: " + wake_test);
   if (Integer.parseInt(System.console().readLine()) != wake_test) {
    System.exit(1);
   }
  } else {
   System.err.println("Must be run from an interactive console.");
   System.exit(1);
  }
  SimpleEntry<String, String>[] edits = new SimpleEntry[]{
   new SimpleEntry("JavaFX", javafx),
   new SimpleEntry("com.sun", "com." + com_sun),
   new SimpleEntry("com_sun", "com_" + com_sun),
   new SimpleEntry("com/sun", "com/" + com_sun),
   new SimpleEntry("com\\sun", "com\\" + com_sun),
   new SimpleEntry("com.oracle", "com." + com_oracle),
   new SimpleEntry("com_oracle", "com_" + com_oracle),
   new SimpleEntry("com/oracle", "com/" + com_oracle),
   new SimpleEntry("com\\oracle", "com\\" + com_oracle)
  };
  SimpleEntry<String, String>[] dir_renames = new SimpleEntry[]{
   new SimpleEntry("javafx", javafx),
   new SimpleEntry("sun", com_sun),
   new SimpleEntry("oracle", com_oracle)
  };
  SimpleEntry<String, String>[] file_renames = new SimpleEntry[]{
   new SimpleEntry("javafx", javafx),
   new SimpleEntry("com_sun", "com_" + com_sun),
   new SimpleEntry("com_oracle", "com_" + com_oracle)
  };
  {
   File conspicuous_file = new File(FilenameUtils.concat(path.toString(),
    ".travis.yml"));
   if (!conspicuous_file.exists()) {
    System.err.println("This does not appear to be an openjdk-JavaFX folder");
    System.exit(1);
   }
   File mysterious_file = new File(FilenameUtils.concat(path.toString(),
    "letter_from_lady_x_re_am_Main.eml"));
   if (!mysterious_file.exists()) {
    System.err.println(
     "Not completely convinced this is a valid openjdk-JavaFX fork folder");
    System.exit(1);
   }
  }
  if (reverse) {
   reverseEntries(edits);
   reverseEntries(dir_renames);
   reverseEntries(file_renames);
  }
  Collection<File> files = getFiles(path);
  files
   .parallelStream()
   .filter(o -> o.isFile())
   .map(o -> new ImmutablePair<>(o, readFile(o)))
   .map(o -> new ImmutableTriple<>(o.left, o.right, edit(o.right, edits)))
   .filter(o -> o.middle != o.right)
   .forEach(o -> writeFile(o.left, o.right));
  moveFiles_to(files
   .stream()
   .filter(o -> o.isFile())
   .collect(Collectors.toList()),
   file_renames);
  moveFiles_to(files
   .stream()
   .filter(o -> o.isDirectory())
   .collect(Collectors.toList()),
   dir_renames);
 }

 private static void printUsage() {
  System.err.println(
   "Usage: rebrand base_path <javafx> <sun> <oracle> [reverse]\n\n"
   + "base_path: path to root folder of JavaFX clone\n"
   + "<javafx> name to replace JavaFX\n"
   + "<sun> name to replace sun\n"
   + "<oracle> name to replace oracle\n"
   + "[reverse] optionally reverse refactoring back to oracle brands.");
 }

 private static Collection<File> getFiles(File path) {
  /*
   * sort the full path from longest to shortest, so renaming folders depth
   * first won't ruin the navigation path.
   *
   */
  Collection<File> files = FileUtils.listFilesAndDirs(path, FILTER_FILES,
   FILTER_FILES)
   .stream()
   .sorted((o1, o2) -> {
    return o2.toString().length() - o1.toString().length();
   })
   .collect(Collectors.toList());
  return files;
 }

 private static void moveFiles_to(Collection<File> dirs,
  SimpleEntry<String, String>[] edits) {
  for (File dir : dirs) {
   String name = dir.getName();
   String new_name = edit(name, edits);
   if (name != new_name) {
    String old_dir = dir.toString();
    String new_dir = old_dir.substring(0, FilenameUtils.indexOfLastSeparator(
     old_dir) + 1) + new_name;
    File new_file = new File(new_dir);
    if (!dir.renameTo(new_file)) {
     System.err.println("Failed to rename " + dir.toString());
     System.exit(1);
    }
   }
  }
 }

 private static String edit(String source, SimpleEntry<String, String>[] edits) {
  String s = source;
  for (SimpleEntry<String, String> edit : edits) {
   s = StringUtils.replaceIgnoreCase(s, edit.getKey(), edit.getValue());
  }
  /*
   * StringUtils.replace returns the source string if no replacement was done
   *
   */
  return s;
 }

 private static String readFile(File o) {
  try {
   return FileUtils.readFileToString(o, Charset.defaultCharset());
  } catch (IOException ex) {
   Logger.getLogger(Rebrand.class.getName()).log(Level.SEVERE, null, ex);
   System.exit(1);
   return null;
  }
 }

 private static void writeFile(File file, String data) {
  try {
   FileUtils.writeStringToFile(file, data, Charset.defaultCharset());
  } catch (IOException ex) {
   Logger.getLogger(Rebrand.class.getName()).log(Level.SEVERE, null, ex);
   System.exit(1);
  }
 }

 private static void reverseEntries(SimpleEntry<String, String>[] entries) {
  for (int i = 0; i < entries.length; i++) {
   SimpleEntry<String, String> entry = entries[i];
   entries[i] = new SimpleEntry<>(entry.getValue(), entry.getKey());
  }
 }

}
