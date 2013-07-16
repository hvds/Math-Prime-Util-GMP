#!/usr/bin/env perl
use warnings;
use strict;
use Math::BigInt lib=>"GMP,Pari";
use Math::Prime::Util qw/:all/;
use Getopt::Long;
$|++;

# MPU certificate verification.
# Written by Dana Jacobsen, 2013.

# Requires Math::Prime::Util v0.30 or later.
# Will be very slow without Math:::Prime::Util::GMP for EC operations.

# Exits with:
#  0  all numbers verified prime
#  1  at least one number verified composite
#  2  incorrect or incomplete conditions.  Cannot verify.
#  3  certificate file cannot be parsed or no number found

my $verbose = 2;
my $quiet;
GetOptions("verbose+" => \$verbose,
           "quiet" => \$quiet,
          ) or die "Error in option parsing\n";
$verbose = 0 if $quiet;

sub error ($) {
  my $message = shift;
  warn "\n$message\n" if $verbose;
  exit(3);  # error in certificate
}

sub fail ($) {
  my $message = shift;
  warn "\n$message\n" if $verbose;
  exit(2);  # Failed a condition
}

my $N;
my %parts;  # Map of "N is prime if Q is prime"
my %proof_funcs = (
  ECPP        =>  \&prove_ecpp,    # Standard ECPP proof
  ECPP3       =>  \&prove_ecpp3,   # Primo type 3
  ECPP4       =>  \&prove_ecpp4,   # Primo type 4
  BLS15       =>  \&prove_bls15,   # basic n+1, includes Primo type 2
  BLS3        =>  \&prove_bls3,    # basic n-1
  BLS5        =>  \&prove_bls5,    # much better n-1
  SMALL       =>  \&prove_small,   # n <= 2^64
  POCKLINGTON =>  \&prove_pock,    # simple n-1, Primo type 1
  LUCAS       =>  \&prove_lucas,   # n-1 completely factored
);
my $smallval = Math::BigInt->new(2)->bpow(64);
my $step = 1;

while (<>) {
  next if /^\s*#/ or /^\s*$/;  # Skip comments and blank lines

  chomp;

  if (defined $N && /^Proof for:/) {
    # Done with this number, starting the next.
    print " " x 60, "\r" if $verbose == 2;
    if (verify_chain($N)) {
      print "PRIME\n" if $verbose;
    } else {
      print "NOT PROVEN\n" if $verbose;
      exit(2);
    }
    undef $N;
    undef %parts;
  }

  if (!defined $N) {
    if (/^Proof for:/) {
      ($N) = read_vars('Proof for', qw/N/);
      if    ($verbose == 1) {  print "N $N";  }
      elsif ($verbose == 2) {  print "$N\n";  }
      if (!is_prob_prime($N)) {
        print "FAILS BPSW\n" if $verbose;
        exit(1);
      }
    } elsif (/^Type /) {
      error("Starting type without telling me the N value!");
    }
    next;
  }

  if (/^Type (.*?)\s*$/) {
    my $type = $1;
    $type =~ tr/a-z/A-Z/;
    error("Unknown type: $type") unless defined $proof_funcs{$type};
    my $q = $proof_funcs{$type}->();
    if    ($verbose == 1) {  print ".";  }
    elsif ($verbose == 2) {  printf "step %2d: %4d digits  type %12s\r", $step++, length($q), $type; }
  }
}
error("No N found") unless defined $N;
print " " x 60, "\r" if $verbose == 2;
if (verify_chain($N)) {
  print "PRIME\n" if $verbose;
  exit(0);
} else {
  print "NOT PROVEN\n" if $verbose;
  exit(2);
}

sub verify_chain {
  my $n = shift;
  die "Internal error: argument not defined" unless defined $n;
  my @qs = ($n);
  while (@qs) {
    my $q = shift @qs;
    # Check that this q has a chain
    if (!defined $parts{$q}) {
      # Auto-small: handle small q right here.
      if ($q <= $smallval) {
        fail "Small q $q doesn't pass BPSW" unless is_prime($q);
        next;
      } else {
        error "q value $q has no proof\n";
      }
    }
    die "Internal error: Invalid parts entry" unless ref($parts{$q}) eq 'ARRAY';
    # q is prime if all it's chains are prime.
    push @qs, @{$parts{$q}};
  }
  1;
}

sub prove_ecpp {
  _ecpp_conditions( read_vars('ECPP', qw/N A B M Q X Y/) );
}
sub prove_ecpp3 {
  my ($n, $s, $r, $a, $b, $t) = read_vars('ECPP3', qw/N S R A B T/);
  fail "ECPP3: $n failed |A| <= N/2" unless abs($a) <= $n/2;
  fail "ECPP3: $n failed |B| <= N/2" unless abs($b) <= $n/2;
  fail "ECPP3: $n failed T >= 0" unless $t >= 0;
  fail "ECPP3: $n failed T < N" unless $t < $n;
  my $l = ($t*$t*$t + $a*$t + $b) % $n;
  _ecpp_conditions(
    $n,
    ($a * $l*$l) % $n,
    ($b * $l*$l*$l) % $n,
    $r*$s,
    $r,
    ($t*$l) % $n,
    ($l*$l) % $n
  );
}
sub prove_ecpp4 {
  my ($n, $s, $r, $j, $t) = read_vars('ECPP4', qw/N S R J T/);
  fail "ECPP4: $n failed |J| <= N/2" unless abs($j) <= $n/2;
  fail "ECPP4: $n failed T >= 0" unless $t >= 0;
  fail "ECPP4: $n failed T < N" unless $t < $n;
  my $a = 3 * $j * (1728 - $j);
  my $b = 2 * $j * (1728 - $j) * (1728 - $j);
  my $l = ($t*$t*$t + $a*$t + $b) % $n;
  _ecpp_conditions(
    $n,
    ($a * $l*$l) % $n,
    ($b * $l*$l*$l) % $n,
    $r*$s,
    $r,
    ($t*$l) % $n,
    ($l*$l) % $n
  );
}

sub prove_bls15 {
  my ($n, $q, $lp, $lq) = read_vars('BLS15', qw/N Q LP LQ/);
  fail "BLS15: $n failed Q odd" unless $q->is_odd();
  fail "BLS15: $n failed Q > 2" unless $q > 2;
  my ($m, $rem) = ($n+1)->copy->bdiv($q);
  fail "BLS15: $n failed Q divides N+1" unless $rem == 0;
  fail "BLS15: $n failed MQ-1 = N" unless $m*$q-1 == $n;
  fail "BLS15: $n failed M > 0" unless $m > 0;
  fail "BLS15: $n failed 2Q-1 > sqrt(N)" unless 2*$q-1 > $n->copy->bsqrt();
  my $D = $lp*$lp - 4*$lq;
  fail "BLS15: $n failed D != 0" unless $D != 0;
  fail "BLS15: $n failed jacobi(D,N) = -1" unless _jacobi($D,$n) == -1;
  fail "BLS15: $n failed V_{m/2} mod N != 0"
      unless (lucas_sequence($n, $lp, $lq, $m/2))[1] != 0;
  fail "BLS15: $n failed V_{(N+1)/2} mod N == 0"
      unless (lucas_sequence($n, $lp, $lq, ($n+1)/2))[1] == 0;
  $parts{$n} = [$q];
  $n;
}
sub prove_bls3 {
  my ($n, $q, $a) = read_vars('BLS3', qw/N Q A/);
  fail "BLS3: $n failed Q odd" unless $q->is_odd();
  fail "BLS3: $n failed Q > 2" unless $q > 2;
  my ($m, $rem) = ($n-1)->copy->bdiv($q);
  fail "BLS3: $n failed Q divides N-1" unless $rem == 0;
  fail "BLS3: $n failed MQ+1 = N" unless $m*$q+1 == $n;
  fail "BLS3: $n failed M > 0" unless $m > 0;
  fail "BLS3: $n failed 2Q+1 > sqrt(n)" unless 2*$q+1 > $n->copy->bsqrt();
  fail "BLS3: $n failed A^((N-1)/2) = N-1 mod N" unless $a->copy->bmodpow(($n-1)/2, $n) == $n-1;
  fail "BLS3: $n failed A^(M/2) != N-1 mod N" unless $a->copy->bmodpow($m/2,$n) != $n-1;
  $parts{$n} = [$q];
  $n;
}
sub prove_pock {
  my ($n, $q, $a) = read_vars('POCKLINGTON', qw/N Q A/);
  my ($m, $rem) = ($n-1)->copy->bdiv($q);
  fail "Pocklington: $n failed Q divides N-1" unless $rem == 0;
  fail "Pocklington: $n failed M is even" unless $m->is_even();
  fail "Pocklington: $n failed M > 0" unless $m > 0;
  fail "Pocklington: $n failed M < Q" unless $m < $q;
  fail "Pocklington: $n failed MQ+1 = N" unless $m*$q+1 == $n;
  fail "Pocklington: $n failed A > 1" unless $a > 1;
  fail "Pocklington: $n failed A^(N-1) mod N = 1"
    unless $a->copy->bmodpow($n-1, $n) == 1;
  fail "Pocklington: $n failed gcd(A^M - 1, N) = 1"
    unless Math::BigInt::bgcd($a->copy->bmodpow($m, $n)-1, $n) == 1;
  $parts{$n} = [$q];
  $n;
}
sub prove_small {
  my ($n) = read_vars('Small', qw/N/);
  fail "Small value n is > 2^64\n" unless $n <= $smallval;
  fail "Small value n does not pass BPSW" unless is_prime($n);
  $parts{$n} = [];
  $n;
}
sub _ecpp_conditions {
  my ($n, $a, $b, $m, $q, $x, $y) = @_;
  fail "ECPP: $n failed N > 0" unless $n > 0;
  fail "ECPP: $n failed gcd(N, 6) = 1" unless Math::BigInt::bgcd($n, 6) == 1;
  fail "ECPP: $n failed gcd(4*a^3 + 27*b^2, N) = 1"
    unless Math::BigInt::bgcd(4*$a*$a*$a+27*$b*$b,$n) == 1;
  fail "ECPP: $n failed Y^2 = X^3 + A*X + B mod N"
    unless ($y*$y) % $n == ($x*$x*$x + $a*$x + $b) % $n;
  fail "ECPP: $n failed M >= N - 2*sqrt(N) + 1" unless $m >= $n - 2*$n->copy->bsqrt() + 1;
  fail "ECPP: $n failed M <= N + 2*sqrt(N) + 1" unless $m <= $n + 2*$n->copy->bsqrt() + 1;
  fail "ECPP: $n failed Q > (N^(1/4)+1)^2" unless $q > $n->copy->broot(4)->badd(1)->bpow(2);
  fail "ECPP: $n failed Q < N" unless $q < $n;
  fail "ECPP: $n failed M != Q" unless $m != $q;
  my ($mdivq, $rem) = $m->copy->bdiv($q);
  fail "ECPP: $n failed Q divides M" unless $rem == 0;

  # Now verify the elliptic curve
  my $correct_point = 0;
  if (0 && prime_get_config->{'gmp'} && defined &Math::Prime::Util::GMP::_validate_ecpp_curve) {
    $correct_point = Math::Prime::Util::GMP::_validate_ecpp_curve($a, $b, $n, $x, $y, $m, $q);
  } else {
    if (!defined $Math::Prime::Util::ECAffinePoint::VERSION) {
      eval { require Math::Prime::Util::ECAffinePoint; 1; }
      or do { die "Cannot load Math::Prime::Util::ECAffinePoint"; };
    }
    my $ECP = Math::Prime::Util::ECAffinePoint->new($a, $b, $n, $x, $y);
    # Compute U = (m/q)P, check U != point at infinity
    $ECP->mul( $m->copy->bdiv($q)->as_int );
    if (!$ECP->is_infinity) {
      # Compute V = qU, check V = point at infinity
      $ECP->mul( $q );
      $correct_point = 1 if $ECP->is_infinity;
    }
  }
  fail "ECPP: $n failed elliptic curve conditions" unless $correct_point;
  $parts{$n} = [$q];
  $n;
}

sub prove_bls5 {
  # No good way to do this using read_vars
  my $n;
  my $index = 0;
  my (@Q, @A);
  $Q[0] = Math::BigInt->new(2);  # 2 is implicit
  while (1) {
    my $line = <>;
    error("end of file during type BLS5") unless defined $line;
    # Skip comments and blank lines
    next if $line =~ /^\s*#/ or $line =~ /^\s*$/;
    # Stop when we see a line starting with -.
    last if $line =~ /^-/;
    chomp($line);
    if ($line =~ /^N\s+(\d+)/) {
      error("BLS5: N redefined") if defined $n;
      $n = Math::BigInt->new("$1");
    } elsif ($line =~ /^Q\[(\d+)\]\s+(\d+)/) {
      $index++;
      error("BLS5: Invalid index: $1") unless $1 == $index;
      $Q[$1] = Math::BigInt->new("$2");
    } elsif ($line =~ /^A\[(\d+)\]\s+(\d+)/) {
      error("BLS5: Invalid index: A[$1]") unless $1 >= 0 && $1 <= $index;
      $A[$1] = Math::BigInt->new("$2");
    } else {
      error("Unrecognized line: $line");
    }
  }
  my $nm1 = $n - 1;
  my $F = Math::BigInt->bone;
  my $R = $nm1->copy;
  foreach my $i (0 .. $index) {
    error "BLS5: $n failed Q[$i] doesn't exist" unless defined $Q[$i];
    $A[$i] = Math::BigInt->new(2) unless defined $A[$i];
    fail "BLS5: $n failed Q[$i] > 1" unless $Q[$i] > 1;
    fail "BLS5: $n failed Q[$i] < N-1" unless $Q[$i] < $nm1;
    fail "BLS5: $n failed A[$i] > 1" unless $A[$i] > 1;
    fail "BLS5: $n failed A[$i] < N" unless $A[$i] < $n;
    fail "BLS5: $n failed Q[$i] divides N-1" unless ($nm1 % $Q[$i]) == 0;
    while (($R % $Q[$i]) == 0) {
      $F *= $Q[$i];
      $R /= $Q[$i];
    }
  }
  die "BLS5: Internal error R != (N-1)/F\n" unless $R == $nm1/$F;
  fail "BLS5: $n failed F is even" unless $F->is_even();
  fail "BLS5: $n failed gcd(F, R) = 1\n" unless Math::BigInt::bgcd($F,$R) == 1;
  my ($s, $r) = $R->copy->bdiv(2*$F);
  my $P = ($F+1) * (2 * $F * $F + ($r-1)*$F + 1);
  fail "BLS5: $n failed n < P" unless $n < $P;
  fail "BLS5: $n failed s=0 OR r^2-8s not a perfect square"
    unless $s == 0 or !_is_perfect_square($r*$r - 8*$s);
  foreach my $i (0 .. $index) {
    my $a = $A[$i];
    my $q = $Q[$i];
    fail "BLS5: $n failed A[i]^(N-1) mod N = 1"
      unless $a->copy->bmodpow($nm1, $n) == 1;
    fail "BLS5: $n failed gcd(A[i]^((N-1)/Q[i])-1, N) = 1"
      unless Math::BigInt::bgcd($a->copy->bmodpow($nm1/$q, $n)-1, $n) == 1;
  }
  $parts{$n} = [@Q];
  $n;
}
sub prove_lucas {
  # No good way to do this using read_vars
  my $index = 0;
  my $n;
  my @Q;
  my $a;
  while (1) {
    my $line = <>;
    error("end of file during type Lucas") unless defined $line;
    # Skip comments and blank lines
    next if $line =~ /^\s*#/ or $line =~ /^\s*$/;
    chomp($line);
    if ($line =~ /^N\s+(\d+)/) {
      error("Lucas: N redefined") if defined $n;
      $n = Math::BigInt->new("$1");
    } elsif ($line =~ /^Q\[(\d+)\]\s+(\d+)/) {
      $index++;
      error("Lucas: Invalid index: $1") unless $1 == $index;
      $Q[$1] = Math::BigInt->new("$2");
    } elsif ($line =~ /^A\s+(\d+)/) {
      $a = Math::BigInt->new("$1");
      last;
    } else {
      error("Unrecognized line: $line");
    }
  }
  fail "Lucas: $n failed A > 1" unless $a > 1;
  fail "Lucas: $n failed A < N" unless $a < $n;
  my $nm1 = $n - 1;
  my $F = Math::BigInt->bone;
  my $R = $nm1->copy;
  fail "Lucas: $n failed A^(N-1) mod N = 1"
    unless $a->copy->bmodpow($nm1, $n) == 1;
  foreach my $i (1 .. $index) {
    error "Lucas: $n failed Q[$i] doesn't exist" unless defined $Q[$i];
    fail "Lucas: $n failed Q[$i] > 1" unless $Q[$i] > 1;
    fail "Lucas: $n failed Q[$i] < N-1" unless $Q[$i] < $nm1;
    fail "Lucas: $n failed Q[$i] divides N-1" unless ($nm1 % $Q[$i]) == 0;
    fail "Lucas: $n failed A^((N-1)/Q[$i]) mod N != 1"
      unless $a->copy->bmodpow($nm1/$Q[$i], $n) != 1;
    while (($R % $Q[$i]) == 0) {
      $F *= $Q[$i];
      $R /= $Q[$i];
    }
  }
  fail("Lucas: $n failed N-1 has only factors Q") unless $R == 1 && $F == $nm1;
  shift @Q;  # Remove Q[0]
  $parts{$n} = [@Q];
  $n;
}


sub read_vars {
  my $type = shift;
  my %vars = map { $_ => 1 } @_;
  my %return;
  while (scalar keys %vars) {
    my $line = <>;
    error("end of file during type $type") unless defined $line;
    # Skip comments and blank lines
    next if $line =~ /^\s*#/ or $line =~ /^\s*$/;
    chomp($line);
    error("Still missing values in type $type") if $line =~ /^Type /;
    if ($line =~ /^(\S+)\s+(-?\d+)/) {
      my ($var, $val) = ($1, $2);
      $var =~ tr/a-z/A-Z/;
      error("Type $type: repeated or inappropriate var: $line") unless defined $vars{$var};
      $return{$var} = $val;
      delete $vars{$var};
    } else {
      error("Unrecognized line: $line");
    }
  }
  # Now return them in the order given, turned into bigints.
  return map { Math::BigInt->new("$return{$_}") } @_;
}





sub _is_perfect_square {
  my($n) = @_;

  if (ref($n) eq 'Math::BigInt') {
    my $mc = int(($n & 31)->bstr);
    if ($mc==0||$mc==1||$mc==4||$mc==9||$mc==16||$mc==17||$mc==25) {
      my $sq = $n->copy->bsqrt->bfloor;
      $sq->bmul($sq);
      return 1 if $sq == $n;
    }
  } else {
    my $mc = $n & 31;
    if ($mc==0||$mc==1||$mc==4||$mc==9||$mc==16||$mc==17||$mc==25) {
      my $sq = int(sqrt($n));
      return 1 if ($sq*$sq) == $n;
    }
  }
  0;
}

# Calculate Jacobi symbol (M|N)
sub _jacobi {
  my($n, $m) = @_;
  return 0 if $m <= 0 || ($m % 2) == 0;
  my $j = 1;
  if ($n < 0) {
    $n = -$n;
    $j = -$j if ($m % 4) == 3;
  }
  # Split loop so we can reduce n/m to non-bigints after first iteration.
  if ($n != 0) {
    while (($n % 2) == 0) {
      $n >>= 1;
      $j = -$j if ($m % 8) == 3 || ($m % 8) == 5;
    }
    ($n, $m) = ($m, $n);
    $j = -$j if ($n % 4) == 3 && ($m % 4) == 3;
    $n = $n % $m;
    $n = int($n->bstr) if ref($n) eq 'Math::BigInt' && $n <= ''.~0;
    $m = int($m->bstr) if ref($m) eq 'Math::BigInt' && $m <= ''.~0;
  }
  while ($n != 0) {
    while (($n % 2) == 0) {
      $n >>= 1;
      $j = -$j if ($m % 8) == 3 || ($m % 8) == 5;
    }
    ($n, $m) = ($m, $n);
    $j = -$j if ($n % 4) == 3 && ($m % 4) == 3;
    $n = $n % $m;
  }
  return ($m == 1) ? $j : 0;
}
