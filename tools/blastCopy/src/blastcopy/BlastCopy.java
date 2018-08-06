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
package blastcopy;

import java.io.File;
import java.io.IOException;
import static java.lang.System.out;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.AbstractMap.SimpleEntry;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.io.FileUtils;

/**
 * Multithreaded file copy for use when copying large number of small files over the network. Doesn't make much of a difference when copying
 * from local disks.
 *
 * @author Gregery Barton
 */
public class BlastCopy {

 /**
  * @param args the command line arguments
  */
 public static void main(String[] args) {
  Options options = new Options();
  options.addRequiredOption("s", "source", true, "source folder");
  options.addRequiredOption("d", "dest", true, "destination folder");
  options.addOption("u", "update", false, "replace files only if source is newer or older.");
  CommandLineParser parser = new DefaultParser();
  CommandLine cmd;
  try {
   cmd = parser.parse(options, args);
  } catch (ParseException ex) {
   HelpFormatter formatter = new HelpFormatter();
   formatter.printHelp("blastCopy", options);
   System.exit(1);
   return;
  }
  Path source = Paths.get(cmd.getOptionValue("s"));
  Path dest = Paths.get(cmd.getOptionValue("d"));
  boolean update = cmd.hasOption("u");
  if (!doCopy(source, dest,update)) {
   System.exit(1);
  }
 }

 /**
  * Copy files using multiple threads. Any existing files are overridden.
  *
  * @param source Source folder to copy from.
  * @param dest Destination folder to copy to.
  * @param update Only replace files if source is newer or older.
  * @return true on success.
  */
 public static boolean doCopy(Path source, Path dest, boolean update) {
  File source_file = new File(source.toString());
  File dest_file = new File(dest.toString());
  System.out.println("Blasting from '" + source + "' to '" + dest + "'");
  if (!source_file.exists()) {
   out.println("Source '" + source + "' does not exist.");
   return false;
  }
  if (!source_file.isDirectory()) {
   out.println("Source '" + source + "' is not a folder.");
   return false;
  }
  if (!dest_file.exists() && !dest_file.mkdirs()) {
   out.println("Could not make destination folder '" + dest + "'");
   return false;
  }
  out.println("discovering files...");
  Stream<SimpleEntry<File, File>> files = recurseFolders(source.toFile())
   .parallelStream()
   .map(o -> new SimpleEntry<>(o, Paths.get(dest.toString(), source.relativize(o.toPath()).toString()).toFile()));
  if (update) {
   files=files.filter(o -> !o.getValue().exists() || (o.getKey().lastModified() != o.getValue().lastModified()));
  }
  List<SimpleEntry<File, File>> copy_map = files
   .collect(Collectors.toList());
  out.println("making folders...");
  copy_map.stream()
   .map(o -> o.getValue().getParentFile())
   .forEach(o -> o.mkdirs());
  out.println("copying files...");
  copy_map.parallelStream()
   .forEach(o -> copyFile(o.getKey(), o.getValue()));
  out.println("finished blastCopy");
  return true;
 }

 private static void copyFile(File source, File dest) {
  try {
   FileUtils.copyFile(source, dest, true);
  } catch (IOException ex) {
   out.println("Could not copy file '" + source + "'");
  }
 }

 private static List<File> recurseFolders(File s) {
  if (!s.isDirectory()) {
   return Arrays.asList(s);
  }
  try {
   return Files.list(s.toPath()).parallel()
    .map(o -> o.toFile())
    .flatMap(o -> recurseFolders(o).stream())
    .collect(Collectors.toList());
  } catch (IOException ex) {
   return Collections.emptyList();
  }
 }

}
