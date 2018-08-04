/*
 * Copyright (c) 2018 Gregery Barton, all rights reserved.
 *
 * Licensed under the Proliferate License, Version 1.0.
 * You may not use this file except in compliance with the license.
 * You may obtain an index of licenses at:
 * https://github.com/gregeryb/proliferated/wiki/Index-of-Licenses
 *
 * This file may be sublicensed for distribution under the
 * terms and conditions of ONE of the licenses listed below. Any
 * modifications made to THIS file must not violate ANY license
 * listed below. Upon agreeing to these terms, from top to bottom,
 * left to right, the license that applies to you is the FIRST license
 * when this file is deployed as standalone, or the EXACT match to the
 * highest priority license in a combined work, if an exact match
 * does not exist then the first license that is COMPATIBLE within
 * the combined work applies.
 *
 * #PostgreSQL  #Upstream      #0BSD          #APSL-2.0      #EPL-2.0
 * #NPOSL-3.0   #BSL-1.0       #Frmworx1.0    #BSD-2-Clause  #MirOS
 * #LGPL-3.0    #NCSA          #Entessa       #CPAL-1.0      #AFL-3.0
 * #GPL-3.0     #ORCLGPL2      #MS-PL         #OSL-3.0       #ZPL-2.0
 * #LiLiQ-P     #UPL           #BSD-3-Clause  #EUPL-1.1      #APL-1.0
 * #Naumen      #ECOS2         #AAL           #BSD-2-Patent  #W3C
 * #LPPL-1.3c   #ECL-2.0       #Sleepycat     #Apache-2.0    #CDDL-1.0
 * #HPND        #NGPL          #RSCPL         #OFL-1.1       #MPL-1.1
 * #LiLiQ-R+    #AGPL-3.0      #RPL-1.5       #Xnet          #WXwindows
 * #LGPL-2.1    #LiLiQ-R       #Zlib          #EPL-1.0       #SimPL-2.0
 * #Motosoto    #MPL-1.0       #RPSL-1.0      #ISC           #Python-2.0
 * #VSL-1.0     #OCLC-2.0      #CNRI-Python   #Fair          #NTP
 * #OSET        #Watcom-1.0    #EUDatagrid    #GPL-2.0       #CATOSL-1.1
 * #MPL-2.0     #LPL-1.02      #NASA-1.3      #SPL-1.0       #CUA-OPL-1.0
 * #MS-RL       #QPL-1.0       #MIT           #EFL-2.0
 * #IPA         #Artistic-2.0  #PHP-3.0       #OGTSL
 * #CECILL-2.1  #Multics       #Nokia         #IPL-1.0
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
   + "\n- Binary files introduced into the folder tree after initial clone from GitHub may also be corrupted.");
  Console console = System.console();
  if (console != null) {
   int wake_test = new Random().nextInt();
   System.err.println("Type the number to confirm: " + wake_test);
   if (Integer.parseInt(System.console().readLine()) != wake_test) {
    System.exit(1);
   }
  } else {

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
   s = StringUtils.replace(s, edit.getKey().toUpperCase(), edit.getValue()
    .toUpperCase());
   s = StringUtils.replace(s, edit.getKey().toLowerCase(), edit.getValue()
    .toLowerCase());
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
