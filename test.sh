#!/bin/bash

assert() {
  input="$1"
  expected="$2"

  ./mincc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" == "$expected" ]
    then
      echo "$input => $actual"
    else
      echo "$input => $expected expected, but got $actual"
      exit 1
  fi
}

assert '42;' 42
assert '123;' 123

assert '1+2;' 3
assert '34+8;' 42

assert '1 +2;' 3
assert '  1+ 2 ;' 3

assert '1+2+3;' 6

assert '6-2;' 4
assert '6+2-3;' 5

assert '6*2;' 12
assert '6*2-2;' 10
assert '2+6*2;' 14
assert '2+6*2-3*2;' 8

assert '6/2;' 3
assert '6/2-1;' 2
assert '1+6/2;' 4
assert '6/2*3;' 9
assert '2*6/2;' 6

assert '3==4;' 0
assert '3==3;' 1

assert '3!=4;' 1
assert '3!=3;' 0

assert '1; 2; 3;' 3

assert 'a=42; a;' 42
assert 'a=42; a+3;' 45
assert 'a=42; z=2; a+z;' 44

assert 'var=34; var;' 34
assert 'foo=12; var=34; foo+var;' 46
assert 'foo=12; faa=34; foo+faa;' 46

echo OK
