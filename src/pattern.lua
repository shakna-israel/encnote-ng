expand_pattern = function(pattern)

	-- :graph: - all printable characters, not including space
	pattern = string.gsub(pattern, "%:graph%:", function(m)
		return string.gsub(expand_pattern(":print:"), ' ', '')
	end)

	-- :print: - all printable characters, including space
	pattern = string.gsub(pattern, "%:print%:", ":alpha::space::xdigit::punct:")

	-- :alnum: - all letters and digits
	pattern = string.gsub(pattern, "%:alnum%:", ":alpha::digit:")

	-- :alpha: - all letters
	pattern = string.gsub(pattern, "%:alpha%:", ":lower::upper:")

	-- :space: - all horizontal or vertical whitespace
	pattern = string.gsub(pattern, "%:space%:", ":blank:\v")

	-- :blank: - all horizontal whitespace
	pattern = string.gsub(pattern, "%:blank%:", "\r\n\t ")

	-- :cntrl: - all control characters
	pattern = string.gsub(pattern, "%:cntrl%:", "\a\b\f\n\r\t\v")

	-- :xdigit: - all hexadecimal digits
	pattern = string.gsub(pattern, "%:xdigit%:", ":digit:ABCDEF")

	-- :digit: - all digits
	pattern = string.gsub(pattern, "%:digit%:", "0123456789")
	
	-- :lower: - all lower case letters
	pattern = string.gsub(pattern, "%:lower%:", "abcdefghijklmnopqrstuvwxyz")

	-- :punct: - all punctuation characters
	pattern = string.gsub(pattern, "%:punct%:", ":apostrophe::bracket::colon::comma::dash::ellipsis::exclamation::period::guillemets::hyphen::question::quote::semicolon::stroke::solidus::interpunct::ampersand::at::asterisk::backslash::bullet::caret::currency::dagger::degree::invexclamation::invquestion::negate::hash::numero::percent::pilcrow::prime::section::tilde::umlaut::underscore::pipe::asterism::index::therefore::interrobang:")

	-- :apostrophe:
	pattern = string.gsub(pattern, "%:apostrophe%:", "`'")

	-- :bracket:
	pattern = string.gsub(pattern, "%:bracket%:", "()[]{}<>")

	-- :colon:
	pattern = string.gsub(pattern, "%:colon%:", ":")

	-- :comma:
	pattern = string.gsub(pattern, "%:comma%:", ",")	

	-- :dash:
	pattern = string.gsub(pattern, "%:dash%:", "‒–—―")

	-- :ellipsis:
	pattern = string.gsub(pattern, "%:ellipsis%:", "…")

	-- :exclamation:
	pattern = string.gsub(pattern, "%:exclamation%:", "!")

	-- :period:
	pattern = string.gsub(pattern, "%:period%:", ".")

	-- :guillemets:
	pattern = string.gsub(pattern, "%:guillemets%:", "«»")

	-- :hyphen:
	pattern = string.gsub(pattern, "%:hyphen%:", "-‐")	

	-- :question:
	pattern = string.gsub(pattern, "%:question%:", "?")

	-- :quote:
	pattern = string.gsub(pattern, "%:quote%:", "‘’“”\"")

	-- :semicolon:
	pattern = string.gsub(pattern, "%:semicolon%:", ";")

	-- :stroke:
	pattern = string.gsub(pattern, "%:stroke%:", "/")

	-- :solidus:
	pattern = string.gsub(pattern, "%:solidus%:", "⁄")

	-- :interpunct:
	pattern = string.gsub(pattern, "%:interpunct%:", "·")

	-- :ampersand:
	pattern = string.gsub(pattern, "%:ampersand%:", "&")

	-- :at:
	pattern = string.gsub(pattern, "%:at%:", "@")

	-- :asterisk:
	pattern = string.gsub(pattern, "%:asterisk%:", "*")

	-- :backslash:
	pattern = string.gsub(pattern, "%:backslash%:", "\\")

	-- :bullet:
	pattern = string.gsub(pattern, "%:bullet%:", "•")

	-- :caret:
	pattern = string.gsub(pattern, "%:caret%:", "^")

	-- :currency:
	pattern = string.gsub(pattern, "%:currency%:", "¤¢$€£¥₩₪")

	-- :dagger:
	pattern = string.gsub(pattern, "%:dagger%:", "†‡")

	-- :degree:
	pattern = string.gsub(pattern, "%:degree%:", "°")

	-- :invexclamation:
	pattern = string.gsub(pattern, "%:invexclamation%:", "¡")

	-- :invquestion:
	pattern = string.gsub(pattern, "%:invquestion%:", "¿")

	-- :negate:
	pattern = string.gsub(pattern, "%:negate%:", "¬")

	-- :hash:
	pattern = string.gsub(pattern, "%:hash%:", "#")

	-- :numero:
	pattern = string.gsub(pattern, "%:numero%:", "№")

	-- :percent:
	pattern = string.gsub(pattern, "%:percent%:", "%%‰‱")

	-- :pilcrow:
	pattern = string.gsub(pattern, "%:pilcrow%:", "¶")

	-- :prime:
	pattern = string.gsub(pattern, "%:prime%:", "′")

	-- :section:
	pattern = string.gsub(pattern, "%:section%:", "§")

	-- :tilde:
	pattern = string.gsub(pattern, "%:tilde%:", "~")

	-- :umlaut:
	pattern = string.gsub(pattern, "%:umlaut%:", "¨")

	-- :underscore:
	pattern = string.gsub(pattern, "%:underscore%:", "_")

	-- :pipe:
	pattern = string.gsub(pattern, "%:pipe%:", "|¦")

	-- :asterism:
	pattern = string.gsub(pattern, "%:asterism%:", "⁂")

	-- :index:
	pattern = string.gsub(pattern, "%:index%:", "☞")
	
	-- :therefore:
	pattern = string.gsub(pattern, "%:therefore%:", "∴")

	-- :interrobang:
	pattern = string.gsub(pattern, "%:interrobang%:", "‽")

	-- :reference:
	pattern = string.gsub(pattern, "%:reference%:", "※")

	-- :upper: - all upper case letters
	pattern = string.gsub(pattern, "%:upper%:", "ABCDEFGHIJKLMNOPQRSTUVWXYZ")

	-- :greek:
	pattern = string.gsub(pattern, "%:greek%:", ":greekupper::greeklower:")

	-- :greeklower:
	pattern = string.gsub(pattern, "%:greeklower%:", "αβγδεζηθικλμνξοπρσςτυφχψω")

	-- :greekupper:
	pattern = string.gsub(pattern, "%:greekupper%:", "ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ")

	-- De-duplicate
	local pats = {}
	for match in string.gmatch(pattern, utf8.charpattern) do
		pats[match] = true
	end

	local pats2 = {}
	for k, _ in pairs(pats) do
		pats2[#pats2 + 1] = k
	end
	table.sort(pats2)

	return table.concat(pats2)
end
