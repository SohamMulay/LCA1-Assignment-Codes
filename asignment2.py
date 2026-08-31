a=int(input("Enter 1st number (a):"))
b=int(input("Enter 2nd number (b):"))
c=int(input("Enter 3rd number (c):"))

if a>=b and a>=c:
    if a == b and a == c:
        print("a,band c are all equal.")
    elif a==b:
        print("a and b are equal.")
    elif a==c:
        print("a and c are equal.")
    else:
        print("a is the greatest")
elif b>=a and b>=c:
    if  b==c:
        print("b and c are all equal.")
    else:
        print("b is the greatest")
else:
    print("c is the greatest")