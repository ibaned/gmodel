Point(10) = {0.000000,1.000000,1.000000,0.500000};
Point(12) = {1.000000,1.000000,1.000000,0.500000};
Point(14) = {1.000000,0.000000,1.000000,0.500000};
Point(16) = {0.000000,0.000000,1.000000,0.500000};
Point(3) = {0.000000,1.000000,0.000000,0.500000};
Point(5) = {1.000000,1.000000,0.000000,0.500000};
Point(1) = {1.000000,0.000000,0.000000,0.500000};
Point(0) = {0.000000,0.000000,0.000000,0.500000};
Line(11) = {3,10};
Line(13) = {5,12};
Line(17) = {0,16};
Line(15) = {1,14};
Line(19) = {16,10};
Line(22) = {10,12};
Line(25) = {14,12};
Line(28) = {16,14};
Line(4) = {0,3};
Line(8) = {3,5};
Line(6) = {1,5};
Line(2) = {0,1};
Line Loop(18) = {4,11,-19,-17};
Line Loop(21) = {8,13,-22,-11};
Line Loop(24) = {6,13,-25,-15};
Line Loop(27) = {2,15,-28,-17};
Line Loop(32) = {28,25,-22,-19};
Line Loop(7) = {2,6,-8,-4};
Point(35) = {0.750000,0.500000,0.500000,0.050000};
Point(34) = {0.250000,0.500000,0.500000,0.050000};
Plane Surface(20) = {18};
Plane Surface(23) = {21};
Plane Surface(26) = {24};
Plane Surface(29) = {27};
Plane Surface(30) = {32};
Plane Surface(9) = {7};
Line(36) = {34,35};
Surface Loop(31) = {-9,30,29,26,-23,-20};
Volume(33) = {31};
Line{36} In Volume{33};
Physical Point(10) = {10};
Physical Point(12) = {12};
Physical Point(14) = {14};
Physical Point(16) = {16};
Physical Point(3) = {3};
Physical Point(5) = {5};
Physical Point(1) = {1};
Physical Point(0) = {0};
Physical Line(11) = {11};
Physical Line(13) = {13};
Physical Line(17) = {17};
Physical Line(15) = {15};
Physical Line(19) = {19};
Physical Line(22) = {22};
Physical Line(25) = {25};
Physical Line(28) = {28};
Physical Line(4) = {4};
Physical Line(8) = {8};
Physical Line(6) = {6};
Physical Line(2) = {2};
Physical Point(35) = {35};
Physical Point(34) = {34};
Physical Surface(20) = {20};
Physical Surface(23) = {23};
Physical Surface(26) = {26};
Physical Surface(29) = {29};
Physical Surface(30) = {30};
Physical Surface(9) = {9};
Physical Line(36) = {36};
Physical Volume(33) = {33};
