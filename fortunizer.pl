#!/usr/bin/perl

use strict;

my $outfile = $ARGV[0];
my $infile = $ARGV[1];

if ($outfile) {
  open(O, ">$outfile") || die("Can't open output file '$outfile'");
} else {
  *O = *STDOUT;
}

if ($infile) {
  open(I, "<$infile") || die("Can't open input file '$infile'");
} else {
  *I = *STDIN;
}

my $str = '';
while(my $l = <I>) {
  if ($l =~ /^\%$/) {
    $str = $1 if ($str =~ /^(.*?)\r?\n?\r?\n-- /sg);
    my @strs = split(/\r?\n/, $str);
    if (@strs < 5) {
      my $s = join(' ',@strs);
      $s =~ s/Usenet/IRC/ig;
      $s =~ s/News lesen/IRC nutzen/ig;
      $s =~ s/_?(d)ieser_? Newsgroup/$1iesem Channel/ig;
      $s =~ s/in _?jeder_? Newsgroup/in jedem Channel/ig;
      $s =~ s/(e)ine Newsgroup/$1in IRC-Channel/ig;
      $s =~ s/(d)iese Newsgroup/$1iesen Channel/ig;
      $s =~ s/Newsgroup-Archiven/Channel-Logs/ig;
      $s =~ s/einer anderen Newsgroup/einem anderen Channel/ig;
      $s =~ s/der Newsgroup/dem Channel/ig;
      $s =~ s/Newsgroup/Channel/ig;
      print O "$s\n";
    }
    $str = '';
  } else {
    $str .= $l;
  }
}

close(I);
close(O);

