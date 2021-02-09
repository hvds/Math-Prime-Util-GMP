#!/usr/bin/env perl
use strict;
use warnings;

use Test::More;
use Math::Prime::Util::GMP qw/
   is_pseudoprime
   is_euler_plumb_pseudoprime
   is_euler_pseudoprime
   is_strong_pseudoprime
   is_lucas_pseudoprime
   is_strong_lucas_pseudoprime
   is_extra_strong_lucas_pseudoprime
   is_almost_extra_strong_lucas_pseudoprime
   is_frobenius_pseudoprime
   is_frobenius_khashin_pseudoprime
   is_frobenius_underwood_pseudoprime
   is_perrin_pseudoprime
   is_prime
   is_bpsw_prime
   lucas_sequence lucasu lucasv
   miller_rabin_random
   primes/;
my $extra = defined $ENV{EXTENDED_TESTING} && $ENV{EXTENDED_TESTING};

# pseudoprimes from 2-100k for many bases

my %pseudoprimes = (
   2 => [ qw/2047 3277 4033 4681 8321 15841 29341 42799 49141 52633 65281 74665 80581 85489 88357 90751 1194649/ ],
   3 => [ qw/121 703 1891 3281 8401 8911 10585 12403 16531 18721 19345 23521 31621 44287 47197 55969 63139 74593 79003 82513 87913 88573 97567/ ],
   5 => [ qw/781 1541 5461 5611 7813 13021 14981 15751 24211 25351 29539 38081 40501 44801 53971 79381/ ],
   7 => [ qw/25 325 703 2101 2353 4525 11041 14089 20197 29857 29891 39331 49241 58825 64681 76627 78937 79381 87673 88399 88831/ ],
  11 => [ qw/133 793 2047 4577 5041 12403 13333 14521 17711 23377 43213 43739 47611 48283 49601 50737 50997 56057 58969 68137 74089 85879 86347 87913 88831/ ],
  13 => [ qw/85 1099 5149 7107 8911 9637 13019 14491 17803 19757 20881 22177 23521 26521 35371 44173 45629 54097 56033 57205 75241 83333 85285 86347/ ],
  17 => [ qw/9 91 145 781 1111 2821 4033 4187 5365 5833 6697 7171 15805 19729 21781 22791 24211 26245 31621 33001 33227 34441 35371 38081 42127 49771 71071 74665 77293 78881 88831 96433 97921 98671/ ],
  19 => [ qw/9 49 169 343 1849 2353 2701 4033 4681 6541 6697 7957 9997 12403 13213 13747 15251 16531 18769 19729 24761 30589 31621 31861 32477 41003 49771 63139 64681 65161 66421 68257 73555 96049/ ],
  23 => [ qw/169 265 553 1271 2701 4033 4371 4681 6533 6541 7957 8321 8651 8911 9805 14981 18721 25201 31861 34133 44173 47611 47783 50737 57401 62849 82513 96049/ ],
  29 => [ qw/15 91 341 469 871 2257 4371 4411 5149 6097 8401 11581 12431 15577 16471 19093 25681 28009 29539 31417 33001 48133 49141 54913 79003/ ],
  31 => [ qw/15 49 133 481 931 6241 8911 9131 10963 11041 14191 17767 29341 56033 58969 68251 79003 83333 87061 88183/ ],
  37 => [ qw/9 451 469 589 685 817 1333 3781 8905 9271 18631 19517 20591 25327 34237 45551 46981 47587 48133 59563 61337 68101 68251 73633 79381 79501 83333 84151 96727/ ],
         61 => [ qw/217 341 1261 2701 3661 6541 6697 7613 13213 16213 22177 23653 23959 31417 50117 61777 63139 67721 76301 77421 79381 80041/ ],
         73 => [ qw/205 259 533 1441 1921 2665 3439 5257 15457 23281 24617 26797 27787 28939 34219 39481 44671 45629 64681 67069 76429 79501 93521/ ],
        325 => [ qw/341 343 697 1141 2059 2149 3097 3537 4033 4681 4941 5833 6517 7987 8911 12403 12913 15043 16021 20017 22261 23221 24649 24929 31841 35371 38503 43213 44173 47197 50041 55909 56033 58969 59089 61337 65441 68823 72641 76793 78409 85879/ ],
       9375 => [ qw/11521 14689 17893 18361 20591 28093 32809 37969 44287 60701 70801 79957 88357 88831 94249 96247 99547/ ],
      28178 => [ qw/28179 29381 30353 34441 35371 37051 38503 43387 50557 51491 57553 79003 82801 83333 87249 88507 97921 99811/ ],
      75088 => [ qw/75089 79381 81317 91001 100101 111361 114211 136927 148289 169641 176661 191407 195649/ ],
     450775 => [ qw/465991 468931 485357 505441 536851 556421 578771 585631 586249 606361 631651 638731 641683 645679/ ],
     642735 => [ qw/653251 653333 663181 676651 714653 759277 794683 805141 844097 872191 874171 894671/ ],
    9780504 => [ qw/9780505 9784915 9826489 9882457 9974791 10017517 10018081 10084177 10188481 10247357 10267951 10392241 10427209 10511201/ ],
  203659041 => [ qw/204172939 204456793 206407057 206976001 207373483 209301121 210339397 211867969 212146507 212337217 212355793 214400629 214539841 215161459/ ],
  553174392 => [ qw/553174393 553945231 554494951 554892787 555429169 557058133 557163157 557165209 558966793 559407061 560291719 561008251 563947141/ ],
 1005905886 => [ qw/1005905887 1007713171 1008793699 1010415421 1010487061 1010836369 1012732873 1015269391 1016250247 1018405741 1020182041/ ],
 1340600841 => [ qw/1345289261 1345582981 1347743101 1348964401 1350371821 1353332417 1355646961 1357500901 1361675929 1364378203 1366346521 1367104639/ ],
 1795265022 => [ qw/1795265023 1797174457 1797741901 1804469753 1807751977 1808043283 1808205701 1813675681 1816462201 1817936371 1819050257/ ],
 3046413974 => [ qw/3046413975 3048698683 3051199817 3068572849 3069705673 3070556233 3079010071 3089940811 3090723901 3109299161 3110951251 3113625601/ ],
 3613982119 => [ qw/3626488471 3630467017 3643480501 3651840727 3653628247 3654142177 3672033223 3672036061 3675774019 3687246109 3690036017 3720856369/ ],
 psp2       => [ qw/341 561 645 1105 1387 1729 1905 2047 2465 2701 2821 3277 4033 4369 4371 4681 5461 6601 7957 8321 8481 8911 10261 10585 11305 12801 13741 13747 13981 14491 15709 15841 16705 18705 18721 19951 23001 23377 25761 29341/ ],
 psp3       => [ qw/91 121 286 671 703 949 1105 1541 1729 1891 2465 2665 2701 2821 3281 3367 3751 4961 5551 6601 7381 8401 8911 10585 11011 12403 14383 15203 15457 15841 16471 16531 18721 19345 23521 24046 24661 24727 28009 29161/ ],
 plumb      => [ qw/1729 1905 2047 2465 3277 4033 4681 8321 12801 15841 16705 18705 25761 29341 33153 34945 41041 42799 46657 49141 52633 65281 74665 75361 80581 85489 87249 88357 90751/ ],
 epsp2      => [ qw/561 1105 1729 1905 2047 2465 3277 4033 4681 6601 8321 8481 10585 12801 15841 16705 18705 25761 29341 30121 33153 34945 41041 42799 46657 49141 52633 62745 65281 74665 75361 80581 85489 87249 88357 90751/ ],
 epsp3      => [ qw/121 703 1729 1891 2821 3281 7381 8401 8911 10585 12403 15457 15841 16531 18721 19345 23521 24661 28009 29341 31621 41041 44287 46657 47197 49141 50881 52633 55969 63139 63973 74593 75361 79003 82513 87913 88573 93961 97567/ ],
 epsp29     => [ qw/15 91 341 469 871 2257 4371 4411 5149 5185 6097 8401 8841 11581 12431 15577 15841 16471 19093 22281 25681 27613 28009 29539 31417 33001 41041 46657 48133 49141 54913 57889 79003 98301/ ],
 lucas      => [ qw/323 377 1159 1829 3827 5459 5777 9071 9179 10877 11419 11663 13919 14839 16109 16211 18407 18971 19043/ ],
 slucas     => [ qw/5459 5777 10877 16109 18971 22499 24569 25199 40309 58519 75077 97439 100127 113573 115639 130139/ ],
 eslucas    => [ qw/989 3239 5777 10877 27971 29681 30739 31631 39059 72389 73919 75077 100127 113573 125249 137549 137801 153931 155819/ ],
 aeslucas1  => [ qw/989 3239 5777 10469 10877 27971 29681 30739 31631 39059 72389 73919 75077 100127 113573 125249 137549 137801 153931 154697 155819/ ],
 aeslucas2  => [ qw/3239 4531 5777 10877 12209 21899 31631 31831 32129 34481 36079 37949 47849 50959 51641 62479 73919 75077 97109 100127 108679 113573 116899 154697 161027/ ],
 perrin     => [ qw/271441 904631 16532714 24658561 27422714 27664033 46672291 102690901 130944133 196075949 214038533 517697641 545670533 801123451/ ],
 frobenius  => [ qw/4181 5777 6721 10877 13201 15251 34561 51841 64079 64681 67861 68251 75077 90061 96049 97921 100127/ ],
 frob35     => [ qw/13333 44801 486157 1615681 3125281 4219129 9006401 12589081 13404751 15576571 16719781/ ],
);
my $num_pseudoprimes = 0;
foreach my $ppref (values %pseudoprimes) {
  $num_pseudoprimes += scalar @$ppref;
}

# 323 and 377 are Lucas pseudoprimes, but not strong Lucas pseudoprimes
my @small_lucas_trials = (2, 9, 16, 100, 102, 323, 377, 2047, 2048, 5781, 9000, 14381);

my %large_pseudoprimes = (
   '75792980677' => [ qw/2/ ],
   '21652684502221' => [ qw/2 7 37 61 9375/ ],
   '3825123056546413051' => [ qw/2 3 5 7 11 13 17 19 23 29 31 325 9375/ ],
   '318665857834031151167461' => [ qw/2 3 5 7 11 13 17 19 23 29 31 37 325 9375/ ],
   '3317044064679887385961981' => [ qw/2 3 5 7 11 13 17 19 23 29 31 37 73 325 9375/ ],
   '6003094289670105800312596501' => [ qw/2 3 5 7 11 13 17 19 23 29 31 37 61 325 9375/ ],
   '59276361075595573263446330101' => [ qw/2 3 5 7 11 13 17 19 23 29 31 37 325 9375/ ],
   '564132928021909221014087501701' => [ qw/2 3 5 7 11 13 17 19 23 29 31 37 325 9375/ ],
);
my $num_large_pseudoprime_tests = 0;
foreach my $psrp (keys %large_pseudoprimes) {
  $num_large_pseudoprime_tests += scalar @{$large_pseudoprimes{$psrp}};
}

my %lucas_sequences = (
  "323 1 1 324" => [0,2,1],
  "323 4 1 324" => [170,308,1],
  "323 4 5 324" => [194,156,115],
  "323 3 1 324" => [0,2,1],
  "323 3 1  81" => [0,287,1],
  "323 5 -1 81" => [153,195,322],
  "49001 25 117 24501" => [20933,18744,19141],
  "18971 10001 -1 4743" => [5866,14421,18970],
  "18970 47 -17 18969" => [17108,10950,17543],

  "4 1 -1 951" => [2,0,3],
  "4 2 -1 951" => [1,2,3],
  "8 1 -1 47" => [1,7,7],
  "8 2 -1 47" => [1,6,7],
  "5 1 -1 0" => [0,2,1],
  "5 2 -1 0" => [0,2,1],
  "5 1 -1 66" => [3,3,1],
  "5 2 -1 66" => [0,3,1],
  "1001 -4 4 50" => [173,827,562],
  "1001 -4 7 50" => [87,457,595],
  "1001 1 -1 50" => [330,486,1],
  "5 1 -1 4" => [3,2,1],
  "3 6 9 36" => [0,0,0],
  "5 10 25 101" => [0,0,0],
  "6 10 25 101" => [5,4,1],
  "3 -6 9 0" => [0,2,1],
);

my @primes128 = (qw/
  216807359884357411648908138950271200947
  339168371495941440319562622097823889491
  175647712566579256193079384409148729569
  213978050035770705635718665804334250861
  282014465653257435172223280631326130957
  285690571631805499387265005140705006349
  197905182544375865664507026666258550257
  257978530672690459726721542547822424119
  271150181404520740107101159842415035273
  262187868871349017397376949493643287923
/);
my @comp128 = (qw/
  331692821169251128612023074084933636563
  291142820834608911820232911620629416673
  222553723073325022732878644722536036431
  325464724689480915638128579172743588243
  326662586910428159613180378374675586479
  197395185602458924846767613337087999977
  194157480002729115387621030269291379439
  180664716097986611402007784149669477223
  248957328957166865967197552940796547567
  276174467950103435998583356206846142651
/);

my @perrint = (
  [271441, 0],
  ["167385219121", 1],
  ["24708862470601", 2],
  ["102690901", 3],
);


plan tests => 0 + 9
                + 3
                + 6
                + $num_pseudoprimes
                + 1  # mr base 2    2-4k
                + 9  # mr with large bases
                + 3  # multiple bases
                + 1  # small extra_strong
                + scalar @small_lucas_trials
                + scalar(keys %lucas_sequences)
                + 7  # lucasu lucasv
              # + $num_large_pseudoprime_tests
                + 18*$extra  # Large Carmichael numbers
                + 2 # M-R-random
                + 5 * scalar(@primes128)  # strong probable prime tests
                + 5 * scalar(@comp128)    # strong probable prime tests
                + 15  # Check Frobenius for small primes
                + 3   # mrr with seed and neg bases
                + 4  *scalar(@perrint);   # Perrin pseudoprime types
                + 0;

eval { is_strong_pseudoprime(2047); };
like($@, qr/no base/i, "is_strong_pseudoprime with no base fails");
eval { no warnings; is_strong_pseudoprime(2047, undef); };
like($@, qr/(defined|empty)/i, "is_strong_pseudoprime with base undef fails");
eval { is_strong_pseudoprime(2047, ''); };
like($@, qr/(positive|empty)/i, "is_strong_pseudoprime with base '' fails");
eval { is_strong_pseudoprime(2047,0); };
like($@, qr/invalid/i, "is_strong_pseudoprime with base 0 fails");
eval { is_strong_pseudoprime(2047,1); };
like($@, qr/invalid/i, "is_strong_pseudoprime with base 1 fails");
eval { is_strong_pseudoprime(2047,-7); };
like($@, qr/positive/i, "is_strong_pseudoprime with base -7 fails");
eval { no warnings; is_strong_pseudoprime(undef, 2); };
like($@, qr/(defined|empty)/i, "is_strong_pseudoprime(undef,2) is invalid");
eval { is_strong_pseudoprime('', 2); };
like($@, qr/(positive|empty)/i, "is_strong_pseudoprime('',2) is invalid");
eval { is_strong_pseudoprime(-7, 2); };
like($@, qr/positive/i, "is_strong_pseudoprime(-7,2) is invalid");

eval { no warnings; is_strong_lucas_pseudoprime(undef); };
like($@, qr/empty string/i, "is_strong_lucas_pseudoprime(undef) is invalid");
eval { is_strong_lucas_pseudoprime(''); };
like($@, qr/empty string/i, "is_strong_lucas_pseudoprime('') is invalid");
eval { is_strong_lucas_pseudoprime(-7); };
like($@, qr/integer/i, "is_strong_lucas_pseudoprime(-7) is invalid");


is( is_strong_pseudoprime(0, 2), 0, "spsp(0, 2) shortcut composite");
is( is_strong_pseudoprime(1, 2), 0, "spsp(1, 2) shortcut composite");
is( is_strong_pseudoprime(2, 2), 1, "spsp(2, 2) shortcut prime");
is( is_strong_pseudoprime(3, 2), 1, "spsp(2, 2) shortcut prime");
is( is_strong_lucas_pseudoprime(1), 0, "slpsp(1) shortcut composite");
is( is_strong_lucas_pseudoprime(3), 1, "slpsp(3) shortcut prime");

# Check that each strong pseudoprime base b makes it through MR with that base
while (my($base, $ppref) = each (%pseudoprimes)) {
  foreach my $p (@$ppref) {
    if      ($base =~ /^psp(\d+)/) {
      my $base = $1;
      ok(is_pseudoprime($p, $base), "$p is a pseudoprime to base $base");
    } elsif ($base =~ /^epsp(\d+)/) {
      my $base = $1;
      ok(is_euler_pseudoprime($p, $base), "$p is an Euler pseudoprime to base $base");
    } elsif ($base =~ /^aeslucas(\d+)/) {
      my $inc = $1;
      ok(is_almost_extra_strong_lucas_pseudoprime($p,$inc), "$p is an almost extra strong Lucas pseudoprime (increment $inc)");
    } elsif ($base eq 'eslucas') {
      ok(is_extra_strong_lucas_pseudoprime($p), "$p is an extra strong Lucas pseudoprime");
    } elsif ($base eq 'slucas') {
      ok(is_strong_lucas_pseudoprime($p), "$p is a strong Lucas-Selfridge pseudoprime");
    } elsif ($base eq 'lucas') {
      ok(is_lucas_pseudoprime($p), "$p is a Lucas-Selfridge pseudoprime");
    } elsif ($base eq 'plumb') {
      ok(is_euler_plumb_pseudoprime($p), "$p is an Euler-Plumb pseudoprime");
    } elsif ($base eq 'perrin') {
      ok(is_perrin_pseudoprime($p), "$p is a Perrin pseudoprime");
    } elsif ($base eq 'frobenius') {
      ok(is_frobenius_pseudoprime($p,1,-1), "$p is a Frobenius (1,-1) pseudoprime");
    } elsif ($base eq 'frob35') {
      ok(is_frobenius_pseudoprime($p,3,-5), "$p is a Frobenius (3,-5) pseudoprime");
    } else {
      ok(is_strong_pseudoprime($p, $base), "Pseudoprime (base $base) $p passes MR");
    }
  }
}

# Verify MR base 2 for all small numbers
{
  my $mr2fail = 0;
  for (2 .. 4032) {
    next if $_ == 2047 || $_ == 3277;
    if (is_prime($_)) {
      if (!is_strong_pseudoprime($_,2)) { $mr2fail = $_; last; }
    } else {
      if (is_strong_pseudoprime($_,2))  { $mr2fail = $_; last; }
    }
  }
  is($mr2fail, 0, "MR base 2 matches is_prime for 2-4032 (excl 2047,3277)");
}

# Verify MR for bases >= n
is( is_strong_pseudoprime(  3,    3), 1, "spsp(  3,    3)");
is( is_strong_pseudoprime( 11,   11), 1, "spsp( 11,   11)");
is( is_strong_pseudoprime( 89, 5785), 1, "spsp( 89, 5785)");
is( is_strong_pseudoprime(257, 6168), 1, "spsp(257, 6168)");
is( is_strong_pseudoprime(367,  367), 1, "spsp(367,  367)");
is( is_strong_pseudoprime(367, 1101), 1, "spsp(367, 1101)");
is( is_strong_pseudoprime(49001, 921211727), 0, "spsp(49001, 921211727)");
is( is_strong_pseudoprime(  331, 921211727), 1, "spsp(  331, 921211727)");
is( is_strong_pseudoprime(49117, 921211727), 1, "spsp(49117, 921211727)");

is(is_pseudoprime(162401, 2, 3, 5, 7, 11, 13), 1, "162401 is a Fermat pseudoprime to bases 2,3,5,7,11,13");
is(is_euler_pseudoprime(1857241, 2, 3, 5, 7, 11, 13), 1, "1857241 is an Euler pseudoprime to bases 2,3,5,7,11,13");
is(is_strong_pseudoprime("3474749660383", 2, 3, 5, 7, 11, 13), 1, "3474749660383 is a strong pseudoprime to bases 2,3,5,7,11,13");

# Verify extra strong for a few small primes
is_deeply( [grep { is_extra_strong_lucas_pseudoprime($_) } 1..100], [@{primes(2,100)}], "The first 100 primes are selected by is_extra_strong_lucas_pseudoprime" );

# Verify Lucas for some small numbers
for my $n (@small_lucas_trials) {
  next if $n == 5459 || $n == 5777 || $n == 10877 || $n == 16109 || $n == 18971;
  if (is_prime($n)) {
    # Technically if it is a prime it isn't a pseudoprime.
    ok(is_strong_lucas_pseudoprime($n), "$n is a prime and a strong Lucas-Selfridge pseudoprime");
  } else {
    ok(!is_strong_lucas_pseudoprime($n), "$n is not a prime and not a strong Lucas-Selfridge pseudoprime");
  }
}

while (my($params, $expect) = each (%lucas_sequences)) {
  my ($n, $p, $q, $k) = split(' ', $params);
  is_deeply( [lucas_sequence($n,$p,$q,$k)], $expect, "Lucas sequence $params" );
}
{
  is( lucasu(1,-1,1001), "70330367711422815821835254877183549770181269836358732742604905087154537118196933579742249494562611733487750449241765991088186363265450223647106012053374121273867339111198139373125598767690091902245245323403501", "Fibonacci(1001)" );
  is( lucasv(1,-1,1001), "157263483085297728693212310227264801375310590871102293547568363266227647954095037360550009174721122072079595635402411260638605742511929970292048335339367003086933714987796078672982630775099044177835579021861251", "Lucas(1001)" );

  # Two examples used in primality proofs by Broadhurst and de Water
  my($n, $str);
  $n = lucasu(9,-1,3671);
  $str = $n;
  substr($str, 15, -15, "...");
  is( $str, "244000543463515...617812356676529", "lucasu(9,-1,3671)" );
  $n = lucasu(287,-1,3079);
  $str = $n;
  substr($str, 15, -15, "...");
  is( $str, "238068250883464...633235555322189", "lucasu(287,-1,3079)" );

  is( lucasv(80,1,71), "1301877113746131144509980743600829473173742739374514641352770534569341664593745336690230666043630253570208103530328102099934784158714320", "lucasv(80,1,71)" );

  $n = lucasv(63,1,13217);
  $str = $n;
  substr($str, 15, -15, "...");
  is( $str, "273696635948516...411235453749183", "lucasv(63,1,13217)" );

  $n = lucasv(10,8,88321);
  $str = $n;
  substr($str, 15, -15, "...");
  is( $str, "580334188745259...957502147624960", "lucasv(10,8,88321)" );
}

if ($extra) {
  my $n;
  # Jacobsen 2015, using Bleichenbacher method
  $n = "341627175004511735787409078802107169251";
  is( is_strong_pseudoprime($n, 2..52), 1, "341..251 is spsp(1..52)" );
  is( is_strong_pseudoprime($n, 53),    0, "341..251 is found composite by base 53" );
  $n = "18424122547908777179569267097345139960751";
  is( is_strong_pseudoprime($n, 2..66), 1, "184..751 is spsp(1..66)" );
  is( is_strong_pseudoprime($n, 67),    0, "184..751 is found composite by base 67" );
  # my $p1 = Math::BigInt->new("2059068011050051");
  # $n = $p1 * (49*($p1-1)+1) * (125*($p1-1)+1);
  $n = "53470982963692324858209985576229320155194985131251";
  is( is_strong_pseudoprime($n, 2..72), 1, "534..251 is spsp(1..72)" );
  is( is_strong_pseudoprime($n, 73),    0, "534..251 is found composite by base 73" );
  # Zhang 2004
  $n = "9688312712744590973050578123260748216127001625571";
  is( is_strong_pseudoprime($n, 2..70), 1, "968..571 is spsp(1..70)" );
  is( is_strong_pseudoprime($n, 71),    0, "968..571 is found composite by base 71" );
  # Bleichenbacher
  $n = "68528663395046912244223605902738356719751082784386681071";
  is( is_strong_pseudoprime($n, @{primes(2,100)}), 1, "685..071 is spsp(1..100)" );
  is( is_strong_pseudoprime($n, 101),    0, "685..071 is found composite by base 101" );
  # Arnault 1994, strong pseudoprime to all bases up to 306
  #my $p1 = Math::BigInt->new("29674495668685510550154174642905332730771991799853043350995075531276838753171770199594238596428121188033664754218345562493168782883");
  #$n = $p1* (313 * ($p1-1) + 1) * (353 * ($p1-1) + 1);
  $n = "2887148238050771212671429597130393991977609459279722700926516024197432303799152733116328983144639225941977803110929349655578418949441740933805615113979999421542416933972905423711002751042080134966731755152859226962916775325475044445856101949404200039904432116776619949629539250452698719329070373564032273701278453899126120309244841494728976885406024976768122077071687938121709811322297802059565867";
  is( is_strong_pseudoprime($n, @{primes(2,306)}), 1, "Arnault-397 Carmichael is spsp(1..306)" );
  is( is_strong_pseudoprime($n, 307), 0, "Arnault-397 Carmichael is found composite by base 307" );

  # Strong pseudoprime to first 100 bases
  $n = "197475704297076769879318479365782605729426528421984294554780711762857505669370986517424096751829488980502254269692200841641288349940843678305321105903510536750100514089183274534482451736946316424510377404498460285069545777656519289729095553895011368091845754887799208568313368087677010037387886257331969473598709629563758316982529541918503729974147573401550326647431929654622465970387164330994694720288156577774827473110333350092369949083055692184067330157343079442986832268281420578909681133401657075403304506177944890621300718745594728786819988830295725434492922853465829752746870734788604359697581532651202427729467";
  is( is_strong_pseudoprime($n, @{primes(2,546)}), 1, "197...467 is spsp(1..546)" );
  is( is_strong_pseudoprime($n, 547), 0, "197...467 is found composite by base 547" );

  # Strong pseudoprime to first 150 bases
  $n = "2504564851231996223418611498583553580586431162725714036294663419005627942030045018144967085826016995812748308972856014960994030057096300272690934546847718913274308904632162037753108744756079484071197757334474410667077275268095650646955133107287726653142089063491101528203219286279619714745365476456016263876476506935633902632378276445418807506643579691598485378380707876204179521868647423847956174718793857337275560326966743283833603259339320266954292802259534246253628396748505321522751546284902630334444060405092248083998624231344891540701484875133564504847404605998928272819163447443703835478321841022013831138852193839499198235152203734163783925867691241340516136948911294063782761240713332883707486122030071233696262539897861764349350102562679795879652984483644711085101952997260985769555028200212433179592354351467963621580674595217679045289986395851940360535530808265791863676735166100444465385760336851651312776349197351443263637225179385994064241024523629682623814114793546441523513505946466820080716059467";
  is( is_strong_pseudoprime($n, @{primes(2,876)}), 1, "250...467 is spsp(1..876)" );
  is( is_strong_pseudoprime($n, 877), 0, "250..467 is found composite by base 877" );
  # Strong pseudoprime to first 168 bases
  # N = (n+1) * (37*n+1) * (41*n+1)
  $n = "2809386136371438866496346539320857607283794588353401165473007394921174159995576890097633348301878401799853448496604965862616291048652062065394646956750323263193037964463262192320342740843556773326129475220032687577421757298519461662249735906372935033549531355723541168448473213797808686850657266188854910604399375221284468243956369013816289853351613404802033369894673267294395882499368769744558478456847832293372532910707925159549055961870528474205973317584333504757691137280936247019772992086410290579840045352494329434008415453636962234340836064927449224611783307531275541463776950841504387756099277118377038405235871794332425052938384904092351280663156507379159650872073401637805282499411435158581593146712826943388219341340599170371727498381901415081480224172469034841153893464174362543356514320522139692755430021327765409775374978255770027259819794532960997484676733140078317807018465818200888425898964847614568913543667470861729894161917981606153150551439410460813448153180857197888310572079656577579695814664713878369660173739371415918888444922272634772987239224582905405240454751027613993535619787590841842251056960329294514093407010964283471430374834873427180817297408975879673867";
  is( is_strong_pseudoprime($n, @{primes(2,1008)}), 1, "280...867 is spsp(1..1008)" );
  is( is_strong_pseudoprime($n, 1009), 0, "280..867 is found composite by base 1009" );
}

{
  my @mrs = map { miller_rabin_random($_, 0) } 0 .. 999;
  my @expect = (1) x 1000;
  is_deeply( [@mrs], [@expect], "Miller-Rabin with 0 random bases" );
}
{
  # This is the number above which fails the first 168 bases.
  my $n = "2809386136371438866496346539320857607283794588353401165473007394921174159995576890097633348301878401799853448496604965862616291048652062065394646956750323263193037964463262192320342740843556773326129475220032687577421757298519461662249735906372935033549531355723541168448473213797808686850657266188854910604399375221284468243956369013816289853351613404802033369894673267294395882499368769744558478456847832293372532910707925159549055961870528474205973317584333504757691137280936247019772992086410290579840045352494329434008415453636962234340836064927449224611783307531275541463776950841504387756099277118377038405235871794332425052938384904092351280663156507379159650872073401637805282499411435158581593146712826943388219341340599170371727498381901415081480224172469034841153893464174362543356514320522139692755430021327765409775374978255770027259819794532960997484676733140078317807018465818200888425898964847614568913543667470861729894161917981606153150551439410460813448153180857197888310572079656577579695814664713878369660173739371415918888444922272634772987239224582905405240454751027613993535619787590841842251056960329294514093407010964283471430374834873427180817297408975879673867";
  # 33 random bases on a 3948 bit number => p < 1e-200.
  # This is why we use k random bases and not the first k bases.
  my $isprime = miller_rabin_random($n, 33);
  is($isprime, 0, "Miller-Rabin with 100 uniform random bases for n returns prime" );
}

for my $p (@primes128) {
  is( is_euler_plumb_pseudoprime($p), 1, "prime $p passes Euler-Plumb primality test");
  is( is_frobenius_pseudoprime($p), 1, "prime $p passes Frobenius primality test");
  is( is_frobenius_khashin_pseudoprime($p), 1, "prime $p passes Frobenius Khashin primality test");
  is( is_frobenius_underwood_pseudoprime($p), 1, "prime $p passes Frobenius Underwood primality test");
  is( is_bpsw_prime($p), 1, "prime $p passes BPSW primality test");
}
for my $p (@comp128) {
  is( is_euler_plumb_pseudoprime($p), 0, "composite $p fails Euler-Plumb primality test");
  is( is_frobenius_pseudoprime($p), 0, "composite $p fails Frobenius primality test");
  is( is_frobenius_khashin_pseudoprime($p), 0, "composite $p fails Frobenius Khashin primality test");
  is( is_frobenius_underwood_pseudoprime($p), 0, "composite $p fails Frobenius Underwood primality test");
  is( is_bpsw_prime($p), 0, "composite $p fails BPSW primality test");
}

# Frobenius has some issues.  Test
for my $p (2,3,5,7,11,13,17,19,23,29,31,37,41,43,47) {
  is( is_frobenius_pseudoprime($p,37,-13), 1, "prime $p is a Frobenius (37,-13) pseudoprime" );
}

# Test miller_rabin_random with a passed in seed value
is(miller_rabin_random("5948714251747610466954817160823054375857",30,"0x6c7b06f2333c8390"), 1, "miller_rabin_random with a seed");
# Test mrr with a negative number of bases
ok(!eval { miller_rabin_random(10007,-4); },   "MRR(10007,-4)");
# Test mrr with too many bases
is(miller_rabin_random(123457,1000000), 1, "miller_rabin_random with excessive base number");

###### Perrin pseudoprimes
for my $pdata (@perrint) {
  my($n, $is) = @$pdata;
  my @name = ('unresticted', 'minimal restricted', 'Adams/Shanks', 'Grantham');
  for my $type (0 .. 3) {
    if ($is >= $type) {
      is( is_perrin_pseudoprime($n, $type), 1, "$n is a $name[$type] Perrin pseudoprime" );
    } else {
      is( is_perrin_pseudoprime($n, $type), 0, "$n is not a $name[$type] Perrin pseudoprime" );
    }
  }
}
