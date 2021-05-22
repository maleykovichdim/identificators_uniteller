#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <mutex>
#include <sstream> 

constexpr unsigned int SIZE_PART = 2;
constexpr unsigned int NUM_PART = 9;

#pragma pack(push, 1)

//struct for pair like "A1" or "Z9"
struct SimpleIdentificator
{
  SimpleIdentificator(){};
  SimpleIdentificator(unsigned char digit, unsigned char letter):digit(digit),letter(letter){};

  static bool IsCheckOK(unsigned char digit, unsigned char letter) {
     if (digit < '1' || digit > '9') return false;
     if (letter < 'A' || letter > 'Z' ||
       std::find(FORBIDDEN.begin(), FORBIDDEN.end(), letter) != FORBIDDEN.end())
         return false;
     return true;  
  }

  bool IsCheckOK() const {
     return IsCheckOK(digit, letter);  
  }

  std::string GetString(bool leftExist = false) const {
    std::stringstream s;
    if (leftExist) s << "-";
    s << letter << digit;
    return s.str();
  }


   bool Set(unsigned char digit = '1', unsigned char letter = 'A'){
     if (!IsCheckOK(digit, letter)) return false;
     this->digit = digit;
     this->letter = letter;
     return true;
   }

   bool IsSet() const noexcept{
     if (digit == 0 || letter == 0) return false;
     return true;
   }

   //return true if we need new left-SimpleIdentificator creation
   //use only for checked object
   bool Increment() noexcept {
     if ((++digit) <= '9') return false;
     digit = 1;
     letter++;
     while (std::find(FORBIDDEN.begin(), FORBIDDEN.end(), letter) != FORBIDDEN.end())
       letter++;
     if (letter <= 'Z') return false;
     return true;
   }

   void SetZero() noexcept{
      digit = 0;
      letter = 0;
   }

   static const std::vector<unsigned char> FORBIDDEN;
   unsigned char digit = 0;
   unsigned char letter = 0;
};

const  std::vector<unsigned char> SimpleIdentificator::FORBIDDEN = { 'D', 'F', 'G', 'J', 'M', 'Q', 'V' }; 


class Identificator
{
public:
  Identificator(){iden[0].Set();}// default = A1

  //multi thread version, non-optimized
  bool SetIdentificatorThreadSafe(const std::string& identificator){
    std::lock_guard<std::mutex> l(_mtx);
    return SetIdentificator(identificator);
  }

  //return true if success set
  inline bool SetIdentificator(std::string identificator){
    identificator.erase(remove(identificator.begin(), identificator.end(), '-'), identificator.end());
    if (identificator.size() > SIZE_PART * NUM_PART ||
        identificator.size() < SIZE_PART ||
        identificator.size() % SIZE_PART > 0)
      return false;

    const unsigned char* input = reinterpret_cast <const unsigned char*>(identificator.c_str());
    int numPart = identificator.size()/SIZE_PART;
    for (int i=0; i<numPart; i++){//check all - transaction approach
      SimpleIdentificator a (input[i*SIZE_PART+1], input[i*SIZE_PART]);
      if (!a.IsCheckOK()) return false;
    }
    for (int i=numPart-1, k=0; i>=0, k<numPart ; i--,k++){
      iden[k].Set(input[i*SIZE_PART+1], input[i*SIZE_PART]);
    }
    for (int i=numPart; i<NUM_PART; i++)
      iden[i].SetZero();
    return true;
  }

  //multi thread version, non-optimized
  std::string GetIdentificatorThreadSafe() const {
    std::lock_guard<std::mutex> l(_mtx);
    return GetIdentificator();
  }

  inline std::string GetIdentificator() const {
    std::ostringstream s;
    bool isFirst = true;
    for(int i = NUM_PART-1; i>=0; i--){
      if(!(iden[i].IsCheckOK())) continue;
      s << iden[i].GetString(!isFirst);
      isFirst = false;
    }
    return s.str();
  }

  //multi thread version, non-optimized
  bool IncrementThreadSafe(){
    std::lock_guard<std::mutex> l(_mtx);
    return Increment();
  }

  //return true if success
  inline bool Increment() noexcept{
    if (IsMaximum()) return false; 
    int i=0;
    for (; i<NUM_PART; i++){
      if (!iden[i].IsSet()){
        iden[i].Set();
        return true;
      }
      bool isNextNeedIncrement = iden[i].Increment();
      if (!isNextNeedIncrement) {
        return true;
      }
      else
      {
        for (int k=i; k>=0; k--){
           iden[k].Set();
        }
      }
    }
    return true;    
  }

protected:

  bool IsMaximum(){
    unsigned char * data = reinterpret_cast<unsigned char *> (iden);
    unsigned char maximum[] = {'9','Z','9','Z','9','Z','9','Z','9','Z','9','Z','9','Z','9','Z','9','Z'};
    if (memcmp(data,maximum, sizeof(maximum)) == 0) return true;
    return false;
  }

  SimpleIdentificator iden[NUM_PART];
  mutable std::mutex _mtx;
};

#pragma pack(pop)


int main()
{

   //TEST
   Identificator a,b;
   std::cout << a.GetIdentificator() << std::endl;
   a.Increment();
   std::cout << a.GetIdentificator() << std::endl;
   a.SetIdentificator("Z9");
   std::cout << a.GetIdentificator() << std::endl;
   a.Increment();
   std::cout << a.GetIdentificator() << std::endl;
   a.Increment();
   std::cout << a.GetIdentificatorThreadSafe() << std::endl;
   a.SetIdentificator("Z9-Z9-Z9-Z9-Z9-Z9-Z9-Z9-Z8");
   std::cout << a.GetIdentificator() << std::endl;
   bool ret = a.Increment();
   std::cout << a.GetIdentificatorThreadSafe() <<" ret="<<ret<< std::endl;
   ret =a.Increment();//BAD - overflow
   std::cout << a.GetIdentificator() <<" ret="<<ret<< std::endl;
   a.SetIdentificator("Z9-Z9-Z9");
   std::cout << a.GetIdentificator() << std::endl;
   ret =a.Increment();
   std::cout << a.GetIdentificator() <<" ret="<<ret<< std::endl;

   //bad cases:
   ret = b.SetIdentificator("Z9-Z9-Z9-Z9-Z9-Z9-Z9-Z9-Z9-Z8");
   std::cout << b.GetIdentificator() <<" ret="<<ret<< std::endl;
   ret = b.SetIdentificator("sdf--d56");
   std::cout << b.GetIdentificatorThreadSafe() <<" ret="<<ret<< std::endl;
   ret = b.SetIdentificatorThreadSafe("Z9-D9");
   std::cout << b.GetIdentificatorThreadSafe() <<" ret="<<ret<< std::endl;   
}



