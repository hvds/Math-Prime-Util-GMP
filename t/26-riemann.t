#!/usr/bin/env perl
use strict;
use warnings;

use Test::More;
use Math::Prime::Util::GMP qw/zeta riemannr li ei/;

my $extra = defined $ENV{EXTENDED_TESTING} && $ENV{EXTENDED_TESTING};

# gp: \p 47   for(s=2,20,print(zeta(s)))
my @zeta = (qw/
1.6449340668482264364724151666460251892189499012
1.2020569031595942853997381615114499907649862923
1.0823232337111381915160036965411679027747509519
1.0369277551433699263313654864570341680570809195
1.0173430619844491397145179297909205279018174900
1.0083492773819228268397975498497967595998635606
1.0040773561979443393786852385086524652589607906
1.0020083928260822144178527692324120604856058514
1.0009945751278180853371459589003190170060195316
1.0004941886041194645587022825264699364686064358
1.0002460865533080482986379980477396709604160885
1.0001227133475784891467518365263573957142751059
1.0000612481350587048292585451051353337474816962
1.0000305882363070204935517285106450625876279487
1.0000152822594086518717325714876367220232373890
1.0000076371976378997622736002935630292130882491
1.0000038172932649998398564616446219397304546972
1.0000019082127165539389256569577951013532585711
1.0000009539620338727961131520386834493459437942/);

# gp:
# r(n)={1+sum(k=1,2000, log(n)^k / (k*k!*zeta(k+1)))}
my %rv = (
       20 => '7.52719634941220484077584239013039997938974169722',
      123 => '30.2234556285623332613428945094834980032607831334',
     1234 => '201.089189397887171164417491080355409507577355431',
    12345 => '1477.18529486278566013620706299851975937829102453',
   123456 => '11602.3885324491433573165310800667605102847042681',
  1234567 => '95364.7282332640388293270946571187905178500286859',
 12345678 => '809199.447325079489265130526492437800991704424795',
123456789 => '7027403.22117008872413898789377520747800808475988',
);

my $zeta_prec_tests = 100;
my $zeta_prec_dig = 1000;
my $zeta_prec_n   = 1000;

plan tests => 1 + scalar(keys %rv) + 2 +
            + ($extra ? (1 + $zeta_prec_tests + 1) : 0)
            + 7 + 5
            + 1    # riemannr non-int
            + 4    # li
            + 3;   # ei

is_deeply( [map { zeta($_,46) } 2 .. 20],
           \@zeta,
           "Zeta(2 .. 20) with 46 digits" );

while (my($n, $expect) = each (%rv)) {
  is( riemannr($n, 48), $expect, "R($n) = $expect" );
}

is(riemannr("1". "0" x 50, 50),
    "876268031750784168878176774217251757122246637483.14", "R(10^50)");
is(riemannr("1". "0" x 150, 150),
    "2903728255736534346416074404628970397529068306125207079032837832579103359053245134597590715919869831753500878534514475936664326796995810651878998118.39", "R(10^150)");
if ($extra) {
  is(riemannr("1". "0" x 350, 350),
    "124238489949931370910732336966515100452049936912035624051962859419809563716547103734387992271397051067462245818951391061505694003378122910328376084414945173986152316160543747767431283218881745289606393942682080649781736046589565713497255284652389254462690055319573972816225055328610744389507159949406453088151820588031068114762784097248028765358622.40", "R(10^350)");
}

if ($extra) {
  # Test rounding
  use Math::BigFloat;
  for (1 .. $zeta_prec_tests) {
    my $n   = 2 + int(rand($zeta_prec_n-1));
    my $dig = 2 + int(rand($zeta_prec_dig-1));
    is( zeta($n,$dig),
        Math::BigFloat->new(zeta($n,$dig+50), $dig+1)->bstr,
        "Zeta($n) with $dig decimal places" );
  }

  # Sum Zeta
  {
    my $sum = Math::BigFloat->new(0);
    for my $s (2 .. 500) {
      $sum += Math::BigFloat->new(zeta($s, 550));
    }
    my $sum501 = substr($sum, 0, 501);
    is( $sum501, "499.99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999969450636365003953179480206067863823002105972594276733361063860907187083734752795422981426273793369615862239495314555272426337205519046466787368647760489165307368863382624392677833345083812263612626144778463640578576092058344947523271193232320203051751308405773804068301164552829334912459718853773108065819531889452863987939391855448068890812517763", "sum of Zeta(2 .. 500) to 500 sig dig" );
  }
}

#########################
# Interesting integer values
is( zeta(  1), undef, "Zeta(1) is undef");
is( zeta(  0), '-0.5000000000000000000000000000000000000000', "Zeta(0) is -0.5");
is( zeta( -1), '-0.0833333333333333333333333333333333333333', "Zeta(-1) is -1/12");
like(zeta( -2),qr/-?0(\.0*)?/, "Zeta(-2) is 0");
is( zeta( -3), '0.0083333333333333333333333333333333333333', "Zeta(-2) is 1/120");
is( zeta(-13), '-0.0833333333333333333333333333333333333333', "Zeta(-13) is -1/12");
is( zeta(-21), '-281.4601449275362318840579710144927536231884', "Zeta(-21) is -77683/276");

# non-integer values

is(zeta(14.8765), '1.0000333260384199701584391778295724313333', "zeta(14.8765)");
is(zeta(0.5,50), '-1.46035450880958681288949915251529801246722933101258', "zeta(0.5)");
is(zeta(-0.5,50), '-0.20788622497735456601730672539704930222626853128767', "zeta(-0.5)");
is(zeta(-1.5,50), '-0.02548520188983303594954298691070474546902498460097', "zeta(-1.5)");
is(zeta(-5.5,50), '-0.00267145801989922459898238195309679265130483737224', "zeta(-5.5)");
#is(zeta(-50.3,50), '1013678293176749028637197.7706625674706214717428202', "zeta(-50.3)");

is(riemannr(123456.78901), "11602.45572750038581778127527882313233341", "riemannr(123456.78901)");

############ li

is(li("1.000000000000000000000000000000000000000000020886719363263",15), "-100.000000000000", "li(1.0000...2088..,15) rounds to -100");
is(li("1.000000000000000000000000000000000000000000020886719363263",25), "-99.99999999999996881196016", "li(1.0000...2088..,25)");
is(li(123456789,71), "7028122.5948718879000781801820670643068975005319034275491399869081030876", "li(123456789,71)");
is(li("1" . "3" x 48, 71), "12143015694679233755826665180456251925892758147.144721437041059310430449", "li(13333....333,71)");

############ ei

is(ei("-.999999",71),"-.21938430227532932486722325953484931808376271553585943741535217668894641", "ei(-.999999)");
is(ei(".0000001",71),"-15.540879886056784427519372537151549799926662195341116860222690550334380", "ei(-.0000001)");
is(ei(123,71),"2147292057663749807443512898798152106344989006252421.1953818484307608884", "ei(123)");
