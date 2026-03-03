#!/usr/bin/env perl

#
# git {log,show} -s --format=%B
#

use strict;
use warnings;

my $E = "[🛑] ";

# 72 characters is a natural choice. It provides 4 characters of
# left/right margin on a standard 80 character wide terminal in
# git-log(1) etc standard output.
#
# vim(1) (from 7.2) ships with Tim Pope's vim-git ftplugin which
# amongst other things autowraps lines when editing commit messages
# after 72 characters.
my $LINE_LENGTH_LIMIT = 72;

my $subject = <>;
my $body;
my $body_line_length_warn = 0;
my $err = 0;

while (<>) {
	$body .= $_;
}

#print $subject;
#print $body;
#print "\n";

sub chk_sub_period {
	# Commits prior to b0067b685 ("Regen after makefile changes.")
	# did not use a period.
	#
	# Some current commits do not use one e.g. 1f57d8dc9
	# ("nginx-1.29.5-RELEASE").
	#
	# Not using a period would be conventional. Email subjects don't
	# generally have it. Headings in texts/documents/web pages etc
	# don't use one.
	print $E . "Subject ends with a period\n" if $subject =~ /.*\.$/;
}

sub chk_sub_length {
	if (length($subject) > $LINE_LENGTH_LIMIT) {
		print $E . "Subject is longer than " . $LINE_LENGTH_LIMIT . " characters\n";
		$err = 1;
	}
}

sub chk_body_blank_line {
	if (($body =~ /^(.*)/)[0]) {
		print $E . "Commit message body should be separated from the subject by a blank line\n";
		$err = 1;
	}
}

sub chk_body_trailers {
	my $prev_line = "";

	foreach (split(/\n/, $body)) {
		if (/^[a-zA-Z-]*: /) {
			if ($prev_line ne "") {
				print $E . "Commit tags/trailers should be separated from the commit message body by a blank line\n";
				$err = 1;
			}

			last;
		}

		$prev_line = $_;
	}
}

sub chk_body_line_length {
	foreach (split(/\n/, $body)) {
		# Ignore indented lines for command/log output etc and URLs.
		if (/^[ \t]/ || /https?:\/\//) {
			next;
		}

		# Stop after hitting commit tags/trailers
		if (/^[a-zA-Z-]*: /) {
			last;
		}

		if (length($_) > $LINE_LENGTH_LIMIT) {
			$body_line_length_warn = 1;
			last;
		}
	}

	if ($body_line_length_warn > 0) {
		print $E . "One or more body lines exceed " . $LINE_LENGTH_LIMIT . " characters. Indent command/log output etc lines to quell this error\n";
		$err = 1;
	}
}

chk_sub_length();
chk_sub_period();

chk_body_blank_line();
chk_body_trailers();
chk_body_line_length();

exit $err;
