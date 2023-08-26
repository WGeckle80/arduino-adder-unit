/*
 * Wyatt Geckle
 * 8/26/23
 * 
 * Using a shift register, the arduino generates 8 bits: 4 for the first
 * operand and 4 for the second.  The user enters the following into the
 * serial monitor: "a + b", "a - b".  The operands a and b must be 4-bit
 * signed integers in range [-8, 7].  The expected output is
 * printed, and physical logic gates perform the operation with a 4-bit
 * output.  Overflow is accounted for in the expected results.
 */


#define OUT_PIN 2
#define CLK_PIN 3
#define ADD_SUB_PIN 4

#define NUM_REGISTER_BITS 8  // 4 bits in each operand
#define MIN_OPERAND_NUM  -8  // -2^(4 - 1)
#define MAX_OPERAND_NUM 7  // 2^(4 - 1) - 1


void numOperation(String strOperation);
void performOperation(int8_t num1, char op, int8_t num2);
void setNumDigits(uint8_t num);


void setup() {
    Serial.begin(9600);
    
    pinMode(OUT_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    pinMode(ADD_SUB_PIN, OUTPUT);
    
    // Clear register
    for (int i = 0; i < NUM_REGISTER_BITS; i++) {
        digitalWrite(CLK_PIN, HIGH);
        delay(1);
        digitalWrite(CLK_PIN, LOW);
        delay(1);
    }

    Serial.print("Enter a valid binary operation with a and b in [");
    Serial.print(MIN_OPERAND_NUM);
    Serial.print(", ");
    delay(50);  // Prints incorrectly without small delay
    Serial.print(MAX_OPERAND_NUM);
    Serial.println("]:");
    Serial.println("\"a + b\", \"a - b\"");
}

void loop() {
    if (Serial.available()) {
        numOperation(Serial.readString());
    }
}


/*
 * Finds operator and operands from string.  Performs the operation.
 */
void numOperation(String strOperation) {
    // Remove whitespace from the operation.
    strOperation.replace(" ", "");
    strOperation.replace("\n", "");
    strOperation.replace("\t", "");

    int operationLength = strOperation.length();

    
    // Finds potential operators in the string by linear search.
    uint8_t numMinusSigns = 0;
    uint8_t numPlusSigns = 0;
    int minusSignLocs[3];
    int plusSignLoc = -1;
    for (uint16_t i = 0; i < operationLength; i++) {
        if (strOperation.charAt(i) == '-') {
            if (numMinusSigns < 3) {
                minusSignLocs[numMinusSigns] = i;
            }
            numMinusSigns++;
        } else if (strOperation.charAt(i) == '+') {
            plusSignLoc = i;
            numPlusSigns++;
        }
    }

    // The following are invalid operations involving addition with
    // examples:
    //   * There is more than 1 plus sign (2 + 2 + 2)
    //   * There are more than 3 minus signs (-2 - --2)
    //   * There is 1 plus sign and 3 minus signs (-2 + --2)
    //   * There is 1 plus sign and 2 minus signs which don't preceed
    //     operands (2 + -2-)
    //   * There is 1 plus sign and 1 minus sign which doesn't preceed
    //     an operand (2 -+ 2)
    bool plusThreeMinus = numPlusSigns == 1 && numMinusSigns == 3;
    bool plusInvalidTwoMinus = numPlusSigns == 1 && numMinusSigns == 2
                               && (minusSignLocs[0] != 0
                                   || minusSignLocs[1] - 1 != plusSignLoc);
    bool plusInvalidOneMinus = numPlusSigns == 1 && numMinusSigns == 1
                               && minusSignLocs[0] != 0
                               && minusSignLocs[0] - 1 != plusSignLoc;

    if (numPlusSigns > 1 || numMinusSigns > 3 || plusThreeMinus
            || plusInvalidTwoMinus || plusInvalidOneMinus) {
        Serial.println("Disallowed Operation");
        return;
    }

    // If adding, perform the operation and return.
    if (numPlusSigns == 1) {
        performOperation(strOperation.substring(0, plusSignLoc).toInt(),
                         '+',
                         strOperation.substring(plusSignLoc + 1).toInt());
        return;
    }


    int minusSignLoc;

    switch (numMinusSigns) {
        case 0:
            Serial.println("Missing Allotted Operator");
            return;
            
        case 1:
            // If both numbers are positive, the minus sign is the
            // first one.
            minusSignLoc = minusSignLocs[0];

            // If the minus sign is in the beginning or end of the
            // string (-22, 22-), there is a missing operand.
            if (minusSignLoc == 0 || minusSignLoc == operationLength - 1) {
                Serial.println("Missing Operand");
                return;
            }

            break;
        
        case 2:
            // If the first minus sign preceeds the first operand and
            // the second one is not the last character, the second
            // minus sign is the operator (-2 - 2).
            // If the first minus sign does not preceed the first
            // operand and the second minus sign is the character
            // after the first, the first one is the operator (2 - -2).
            // If neither are are the case, there's a missing operand.
            if (minusSignLocs[0] == 0
                && minusSignLocs[1] != operationLength - 1) {
                minusSignLoc = minusSignLocs[1];
            } else if (minusSignLocs[0] != 0
                       && minusSignLocs[1] - 1 == minusSignLocs[0]
                       && minusSignLocs[1] != operationLength - 1) {
                minusSignLoc = minusSignLocs[0];
            } else {
                Serial.println("Invalid Operation");
                return;
            }
            break;
            
        case 3:
            // If both numbers are negative, the minus sign is the
            // second one.
            minusSignLoc = minusSignLocs[1];

            // If either negative sign doesn't preceed the operands
            // (2---2, 2--2-), the operation is invalid.
            if (minusSignLocs[0] != 0
                || minusSignLocs[2] - 1 != minusSignLocs[1]
                || minusSignLocs[2] == operationLength - 1) {
                Serial.println("Disallowed Operation");
                return;
            }

            break;
    }

    performOperation(strOperation.substring(0, minusSignLoc).toInt(),
                     '-',
                     strOperation.substring(minusSignLoc + 1).toInt());
}


/*
 * Sends operand bits to shift register, sets
 * addition/subtraction pin, and prints expected output.
 */
void performOperation(int8_t num1, char numOperator, int8_t num2) {
    if (num1 < MIN_OPERAND_NUM || num1 > MAX_OPERAND_NUM
        || num2 < MIN_OPERAND_NUM || num2 > MAX_OPERAND_NUM) {
        Serial.print("Operand(s) Out of Range [");
        Serial.print(MIN_OPERAND_NUM);
        delay(50);  // Prints incorrectly without small delay
        Serial.print(", ");
        Serial.print(MAX_OPERAND_NUM);
        Serial.println("].");
        
        return;
    }
        
    setNumDigits(num2);
    setNumDigits(num1);
    
    Serial.print(num1);
    Serial.print(numOperator);
    Serial.print(num2);
    Serial.print('=');
    
    // The only available operations are addition and
    // subtraction.
    if (numOperator == '+') {
        digitalWrite(ADD_SUB_PIN, LOW);
    } else {
        digitalWrite(ADD_SUB_PIN, HIGH);
        num2 = ~num2 + 1;  // 2's complement
    }

    int8_t result = num1 + num2;

    // Account for overflow in the expected result.
    if (result < MIN_OPERAND_NUM) {
        // If dealing with a 4-bit result, only the 3 least significant
        // bits represent the value.  The most significant bit is
        // guaranteed to be 0 when this overflow occurs.  The leading
        // bits in the corresponding 8-bit number needs to be 0.
        // Bitwise AND with 0b00000111 accomplishes that.
        result &= MAX_OPERAND_NUM;
    } else if (result > MAX_OPERAND_NUM) {
        // If dealing with a 4-bit result, only the 3 least significant
        // bits represent the value.  The most significant bit is
        // guaranteed to be 1 when this overflow occurs.  In order for
        // the corresponding 8-bit number to be negative, bits 5-8 need
        // to be 1.  Bitwise OR with the complement of 0b00000111
        // accomplishes that.
        result |= ~MAX_OPERAND_NUM;
    }
    
    Serial.println(result);
}


/*
 * Sets a single operand's bits in the shift register.
 *
 * NOTE: Be sure to always set the digits of num2 before those of num1.
 */
void setNumDigits(int8_t num) {
    for (uint8_t i = 0; i < NUM_REGISTER_BITS >> 1; i++) {
        digitalWrite(OUT_PIN, num & 1);
        delay(1);
        digitalWrite(CLK_PIN, HIGH);
        delay(1);
        digitalWrite(CLK_PIN, LOW);
        delay(1);
        
        num >>= 1;
    }
}
