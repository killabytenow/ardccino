#!/usr/bin/perl
###############################################################################
# clierrs.list
#
# Parse some input files and generate automated source code.
#
# ---------------------------------------------------------------------------
# ardccino - Arduino dual PWM/DCC controller
#   (C) 2013-2015 Gerardo García Peña <killabytenow@gmail.com>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 3 of the License, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
#   for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
###############################################################################

use strict;
use warnings;
use Data::Dumper qw(Dumper);
use POSIX;

my %files;

sub read_file
{
	my $file = shift;
	my $fh;

	die "File '$file' does not exist."
		if(!exists($files{$file}));

	open($fh, "<", $files{$file}[0]) or die "Cannot open '$files{$file}[0]': $!";

	return $fh;
}

sub create_header
{
	my $file = shift;
	my $fh;

	open($fh, ">", $files{$file}[1]) or die "Cannot open '$files{$file}[1]': $!";

	print $fh "// ** THIS FILE IS AUTOGENERATED - DO NOT EDIT **\n";
	print $fh "// Source: $files{$file}[0]\n\n";
	print $fh "#include \"config.h\"\n\n";

	return $fh;
}

#==============================================================================
# BUILD .tokens.h
#==============================================================================

sub build_tokens_h__build_tree
{
	my ($t, $td) = @_;

	$td->{_rest} = $td->{token} if(!exists($td->{_rest}));
	my $c = substr($td->{_rest}, 0, 1);
	$td->{_rest} = substr($td->{_rest}, 1);

	if(length($td->{_rest}) == 0) {
		die "Cannot insert token '$td->{token}'."
			if(exists($t->{$c}));
		$td->{def} = uc($td->{token});
		$td->{def} =~ s/-/_/g;
		$t->{$c} = $td;
	} else {
		die "Cannot insert token '$td->{token}' (collides with $t->{$c}->{token})."
			if(exists($t->{$c}->{_rest}));
		build_tokens_h__build_tree($t->{$c}, $td);
	}
}

sub build_tokens_h__fix_tree
{
	my ($t, $prefix, $lc) = @_;

	$prefix = "" if(!defined($prefix));
	$lc = ""     if(!defined($lc));

	if(exists($t->{token})) {
		delete $t->{_rest};
		$t->{_prefix} = $prefix;
		return $t;
	}

	return build_tokens_h__fix_tree(
			(values %{$t})[0],
			"$prefix$lc" =~ /^[0-9]+$/
				? ($prefix . $lc . (keys %{$t})[0], "")
				: ($prefix, $lc . (keys %{$t})[0])
		)
		if(scalar(keys %{$t}) == 1);

	$t->{$_} = build_tokens_h__fix_tree($t->{$_}, $prefix . $lc . $_, "")
		foreach (keys %{$t});

	return $t;
}

sub build_tokens_h__print_list_r
{
	my ($t, $fo, $curr_id, $max_len) = @_;

	foreach my $n (map { $t->{$_} } sort keys %{$t}) {
		if(exists($n->{token})) {
			$n->{id} = $$curr_id++;
			$$max_len = length($n->{token}) if(length($n->{token}) > $$max_len);
			print $fo "#define CLI_TOKEN_$n->{def} $n->{id}\n";
			print $fo "PROGMEM const char cli_token_$n->{id}\[]\ = \"$n->{token}\";\n";
		} else {
			build_tokens_h__print_list_r($n, $fo, $curr_id, $max_len);
		}
	}
}

sub build_tokens_h__print_symtab_r
{
	my ($t, $fo) = @_;

	foreach my $n (map { $t->{$_} } sort keys %{$t}) {
		if(exists($n->{token})) {
			#print $fo "cli_token_$n->{id},\n";
			printf $fo "\t{ cli_token_%d, %d },\t// %-5s => %s\n",
					$n->{id},
					length($n->{_prefix}),
					$n->{_prefix},
					$n->{token};
		} else {
			build_tokens_h__print_symtab_r($n, $fo);
		}
	}
}

sub build_tokens_h__print
{
	my ($t, $fo) = @_;
	my ($curr_id, $max_len);
	
	print $fo "// special tokens\n";
	print $fo "#define CLI_TOKEN_INT     100\n";
	print $fo "#define CLI_TOKEN_UINT    101\n";
	print $fo "#define CLI_TOKEN_STRING  102\n";
	print $fo "\n";

	$curr_id = 0;
	$max_len = 0;

	build_tokens_h__print_list_r($t, $fo, \$curr_id, \$max_len);

	printf $fo "\n#define CLI_TOKEN_MAX_LEN %d\n\n", $max_len + 1;

	#print $fo "PROGMEM const char * const cli_tokens[] = {\n";
	#build_tokens_h__print_symtab_r($t, $fo);
	#print $fo "};\n";

	print $fo "typedef struct PROGMEM {\n";
	print $fo "	const char * const token;\n";
	print $fo "	uint8_t            minlen;\n";
	print $fo "} cli_token_t;\n\n";
	print $fo "const cli_token_t cli_tokens[] = {\n";
	build_tokens_h__print_symtab_r($t, $fo);
	print $fo "};\n";
}

sub build_tokens_h
{
	my ($fo, $fi, $tokens);

	# load tokens
	$fi = read_file "tokens";
	$tokens = { };
	while(my $l = <$fi>) {
		chomp $l;
		next if($l =~ /^\s*(#.*)?$/); # ignore comments

		die "Bad line <$l>"
			if($l !~ /^\s*(?<token>[^\s]+)\s*(?:#.*)?$/);

		# add token info
		build_tokens_h__build_tree($tokens, { token => $+{token} });
	}
	close($fi);

	# fix tree
	$tokens = build_tokens_h__fix_tree($tokens);

	# dump tokens
	$fo = create_header "tokens";
	build_tokens_h__print($tokens, $fo);
	close($fo);
}

#==============================================================================
# BUILD .clierrs.h
#==============================================================================

sub build_clierrs_h
{
	my ($fo, $fi, %errors, $n, $max_len);

	# read errors
	$fi = read_file "clierrs";
	$max_len=0;
	while(my $l = <$fi>) {
		chomp $l;
		next if($l =~ /^\s*(#.*)?$/); # ignore comments

		die "Bad line <$l>"
			if($l !~ /^\s*(?<err>[^\s=]+)\s*=\s*(?<str>.+?)\s*$/);

		$errors{$+{err}} = { str => $+{str}, id => scalar(keys %errors) };
		$max_len = length($+{str}) if(length($+{str}) > $max_len);
	}
	close($fi);

	# print strings
	$fo = create_header "clierrs";

	print $fo "#define $_ $errors{$_}->{id}\n"
		foreach(sort { $errors{$a}->{id} <=> $errors{$b}->{id} } keys %errors);
	print $fo "\n";

	print $fo "PROGMEM const char cli_err_$_->{id}\[\] = \"$_->{str}\";\n"
		foreach(sort { $a->{id} <=> $b->{id} } values %errors);
	printf $fo "#define CLI_ERRS_MAX_LEN %d\n\n", $max_len + 1;

	print $fo "PROGMEM const char * const cli_errs[] = {\n";
	for(my $i = 0; $i < scalar(keys %errors); $i++) {
		print $fo "	cli_err_$i,\n"
	}
	print $fo "};\n\n";

	close($fo);
}

#==============================================================================
# BUILD .banner_wide.h and .banner.h
#==============================================================================

sub build_text_h
{
	my $file = shift;
	my ($fo, $fi, @lines, $max_len);

	# read text
	$fi = read_file $file;
	$max_len=0;
	while(my $l = <$fi>) {
		chomp $l;
		$max_len = length($l) if(length($l) > $max_len);
		$l =~ s/"/\\"/g;
		push(@lines, $l);
	}
	close($fi);

	# print
	$fo = create_header $file;

	for(my $i = 0; $i < scalar(@lines); $i++) {
		printf $fo "PROGMEM const char %s_%d[] = \"%s\";\n", $file, $i, $lines[$i];
	}
	print $fo "\n";

	printf $fo "#define %s_MAX_LEN %d\n\n", $file, $max_len;

	printf $fo "PROGMEM const char * const %s[] = {\n", $file;
	for(my $i = 0; $i < scalar(@lines); $i++) {
		printf $fo "	%s_%d,\n", $file, $i;
	}
	print $fo "};\n\n";
	close($fo);
}

#==============================================================================
# BUILD .banner_wide.h and .banner.h
#==============================================================================

sub build_date_h
{
	my $file = shift;
	my ($fo, $fi, @lines, $max_len);

	# print
	$fo = create_header $file;

	printf $fo "PROGMEM const char %s[] = \"(built on %s)\\n\";\n", $file, strftime("%d/%m/%Y %H:%M:%S", localtime);
	print $fo "\n";

	close($fo);
}

#==============================================================================
# MAIN
#==============================================================================

%files = (
	tokens      => [ "tokens.list"      => "auto_tokens.h",      \&build_tokens_h  ],
	clierrs     => [ "clierrs.list"     => "auto_clierrs.h",     \&build_clierrs_h ],
	banner      => [ "banner.txt"       => "auto_banner.h",      \&build_text_h    ],
	banner_wide => [ "banner_wide.txt"  => "auto_banner_wide.h", \&build_text_h    ],
	clihelp     => [ "clihelp.txt"      => "auto_clihelp.h",     \&build_text_h    ],
	build_date  => [ "DATE"             => "auto_build_date.h",  \&build_date_h    ],
);

my @targets = scalar(@ARGV) ? @ARGV : keys %files;

foreach my $target (@targets) {
	die "Target '$target' does not exists."
		if(!exists($files{$target}));
	printf STDERR "%s: Building '%s' (using '%s')\n",
			$target,
			$files{$target}[1],
			$files{$target}[0];
	$files{$target}->[2]->($target);
}

exit 0
