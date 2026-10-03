#include "locator_input.hpp"
#include <stdexcept>

namespace hamclock {
namespace {
unsigned letter(unsigned char character,unsigned maximum) {
    if(character>='a' && character<='z')character=static_cast<unsigned char>(character-'a'+'A');
    if(character<'A' || character>maximum)throw std::invalid_argument("Invalid Maidenhead letter pair");
    return character-'A';
}
unsigned digit(unsigned char character) {
    if(character<'0' || character>'9')throw std::invalid_argument("Invalid Maidenhead square pair");
    return character-'0';
}
}
Location location_from_maidenhead(const std::string& locator) {
    // Check length before parsing or making a copy; accepted input is <=6 bytes.
    if(locator.size()!=2 && locator.size()!=4 && locator.size()!=6)
        throw std::invalid_argument("Maidenhead input must contain 2, 4, or 6 ASCII characters");
    const unsigned field_x=letter(static_cast<unsigned char>(locator[0]),'R');
    const unsigned field_y=letter(static_cast<unsigned char>(locator[1]),'R');
    unsigned square_x=0,square_y=0,sub_x=0,sub_y=0;
    if(locator.size()>=4){square_x=digit(static_cast<unsigned char>(locator[2]));square_y=digit(static_cast<unsigned char>(locator[3]));}
    if(locator.size()==6){sub_x=letter(static_cast<unsigned char>(locator[4]),'X');sub_y=letter(static_cast<unsigned char>(locator[5]),'X');}
    // Calculate integral subdivision units first to avoid accumulating floating
    // additions. A double approximates a rational corner; it is not nudged to
    // the centre/interior simply to influence a subsequent grid encoder.
    const double longitude=static_cast<double>(field_x*240U+square_x*24U+sub_x)/12.0-180.0;
    const double latitude=static_cast<double>(field_y*240U+square_y*24U+sub_y)/24.0-90.0;
    return {latitude,longitude};
}

} // namespace hamclock
