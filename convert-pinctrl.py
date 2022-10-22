i = open ("input", "r")

data = i.readlines()

for line in data:
        line = line.strip()
        a = line.split("(")

        match a[0]:
                case "EXYNOS8_PIN_BANK_EINTN":
                        #print("type: eintn")
                        b = a[1].split(",")
                        result = "EXYNOS850_PIN_BANK_EINTN(" + b[1][1:] + "," + b[2] + "," + b[3] + ","
                        print(result)
                case "EXYNOS9_PIN_BANK_EINTW":
                        #print("type: eintw")
                        b = a[1].split(",")
                        result = "EXYNOS850_PIN_BANK_EINTW(" + b[1][1:] + "," + b[2] + "," + b[3] + "," + b[4] + "),"
                        print(result)
                case "EXYNOS9_PIN_BANK_EINTG":
                        #print("type: eintg")
                        b = a[1].split(",")
                        result = "EXYNOS850_PIN_BANK_EINTG(" + b[1][1:] + "," + b[2] + "," + b[3] + "," + b[4] + "),"
                        print(result)
                case _:
                        print("Unknown type: ", a)
                        exit(1)
