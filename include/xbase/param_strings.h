#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace xsdk::xbase {

/**
 * @brief Error codes reported by ParseParamString().
 */
enum class ParamParseErrorCode {
    /**
     * @brief Null C string was passed to ParseParamString(const char*).
     */
    kNullInput,

    /**
     * @brief An item started with '=' or a quoted key was empty.
     */
    kEmptyKey,

    /**
     * @brief Quoted key was started but no matching closing quote was found.
     */
    kUnterminatedQuotedKey,

    /**
     * @brief Quoted value was started but no matching closing quote was found.
     */
    kUnterminatedQuotedValue,

    /**
     * @brief Unexpected character was found after a quoted key.
     */
    kUnexpectedCharacter
};

/**
 * @brief Non-owning token view into the source text.
 *
 * @details
 * The parser returns key/value/flag tokens as std::string_view in order to avoid
 * per-token string allocations.
 *
 * The token is represented by:
 * - text: substring view
 * - begin: token start offset in the source string
 * - end: one-past-end offset in the source string
 *
 * For quoted keys and quoted values, the token range points to the content
 * inside the outer quotes. The opening and closing quote characters are not
 * included into text, begin, or end.
 *
 * Lifetime rules:
 * - if ParamParseOptions::copy_input is true, all returned views point to the
 *   owned buffer exposed by ParamParseResult::StorageShared();
 * - if ParamParseOptions::copy_input is false, all returned views point to the
 *   input buffer, which must remain alive while the result is used.
 */
struct ParamTokenView {
    std::string_view text;
    size_t           begin = 0;
    size_t           end   = 0;

    /**
     * @brief Returns true if the token is empty.
     */
    bool Empty() const { return text.empty(); }
};

/**
 * @brief Parsed key=value item.
 */
struct ParamStringItem {
    ParamTokenView key;
    ParamTokenView value;
};

/**
 * @brief Parsed flag token without '='.
 *
 * @details
 * Example:
 * @code
 * faststart
 * @endcode
 * is returned as ParamStringFlag rather than as an error. The upper layer may
 * later decide whether flags are allowed or should be treated as invalid input.
 */
struct ParamStringFlag {
    ParamTokenView key;
};

/**
 * @brief Parser error with source position and optional token context.
 */
struct ParamParseError {
    ParamParseErrorCode code     = ParamParseErrorCode::kUnexpectedCharacter;
    size_t              position = 0;

    /**
     * @brief Optional recognized key related to the error.
     */
    ParamTokenView key;

    /**
     * @brief Optional offending source fragment.
     */
    ParamTokenView token;
};

/**
 * @brief Parser options.
 */
struct ParamParseOptions {
    /**
     * @brief If true, the parser stores an internal copy of the input text.
     *
     * @details
     * This is the default and safest mode. All returned std::string_view values
     * then reference the owned storage exposed by ParamParseResult.
     *
     * If false, the parser performs zero-copy parsing and all returned views
     * reference the original input buffer.
     */
    bool copy_input = true;

    ParamParseOptions() = default;

    explicit ParamParseOptions(bool _copy_input) : copy_input(_copy_input) {}
};

/**
 * @brief Result of ParseParamString().
 *
 * @details
 * The parser is soft and always returns a result object.
 *
 * The result may contain:
 * - parsed items in @ref items
 * - parsed flags in @ref flags
 * - collected errors in @ref errors
 *
 * Parsing continues after recoverable errors whenever possible. For example,
 * a broken token after an empty-key error may be skipped while later valid
 * tokens are still returned. Unterminated quoted keys and values are not recoverable in
 * this parser version and stop parsing after the error is reported.
 */
struct ParamParseResult {
    using StoragePtr = std::shared_ptr<const std::string>;

    ParamParseResult() = default;

    explicit ParamParseResult(StoragePtr&& _storage_sp) : storage_sp_(std::move(_storage_sp)) {}

public:
    /**
     * @brief Returns shared ownership of the copied input buffer, if any.
     */
    const StoragePtr& StorageShared() const { return storage_sp_; }

    /**
     * @brief Returns true if the result owns a copied input buffer.
     */
    bool OwnsInput() const { return static_cast<bool>(storage_sp_); }

    /**
     * @brief Returns the source text used by parsed token views.
     *
     * @param _original_input Original input text to return in zero-copy mode.
     *
     * @details
     * If the result owns copied input, returns a view of the owned storage.
     * Otherwise returns `_original_input`.
     *
     * In zero-copy mode `_original_input` must be the same input buffer that was
     * passed to ParseParamString().
     */
    std::string_view SourceView(const std::string_view _original_input) const
    {
        return storage_sp_ ? std::string_view(*storage_sp_) : _original_input;
    }

    /**
     * @brief Parsed key=value items.
     */
    std::vector<ParamStringItem> items;

    /**
     * @brief Parsed standalone flags without '='.
     */
    std::vector<ParamStringFlag> flags;

    /**
     * @brief Collected parser errors.
     */
    std::vector<ParamParseError> errors;

    /**
     * @brief Returns true if at least one error was collected.
     */
    bool HasErrors() const { return !errors.empty(); }

    /**
     * @brief Returns true if neither items nor flags were parsed.
     */
    bool Empty() const { return items.empty() && flags.empty(); }

private:
    StoragePtr storage_sp_;
};

/**
 * @brief Parses a VideoSDK-style parameter string into items and flags.
 *
 * @param _text Input parameter string.
 * @param _options Parser options.
 * @return Soft parse result containing parsed tokens and collected errors.
 *
 * @details
 * Grammar summary:
 * - `key=value` produces ParamStringItem;
 * - `flag` produces ParamStringFlag;
 * - key may be either bare or quoted;
 * - bare key is read until whitespace or '=';
 * - quoted key may use either single or double quotes;
 * - outer key quotes are not included into the returned key token;
 * - whitespace before '=' is allowed;
 * - whitespace after '=' is allowed;
 * - if whitespace after '=' is followed by another assignment token, the
 *   current item gets an empty value and the next assignment is parsed as a
 *   separate item;
 * - unquoted value is read until next whitespace;
 * - quoted value may use either single or double quotes;
 * - outer value quotes are not included into the returned value token;
 * - escape sequences are not decoded;
 * - inside quoted key or value, backslash only prevents the next character from
 *   acting as a closing quote;
 * - any character after backslash inside a quoted token is treated as escaped
 *   for the purpose of quote matching, including another backslash or a quote;
 * - whitespace normally separates tokens, but is not required after a quoted
 *   value if the next token starts immediately after the closing quote.
 *
 * Examples:
 * @code
 * format='mp4' video::codec='q264sw' faststart
 * @endcode
 *
 * Produces:
 * - item: format -> mp4
 * - item: video::codec -> q264sw
 * - flag: faststart
 *
 * Example:
 * @code
 * a='x'b=1
 * @endcode
 *
 * Produces:
 * - item: a -> x
 * - item: b -> 1
 *
 * Example:
 * @code
 * "video::codec"='q264sw' a= b=2
 * @endcode
 *
 * Produces:
 * - item: video::codec -> q264sw
 * - item: a -> empty value
 * - item: b -> 2
 *
 * Example:
 * @code
 * a='x\\'
 * @endcode
 *
 * Produces one item with key `a` and value text `x\\`. The first backslash
 * escapes the second backslash for quote matching only, then the following quote
 * closes the value.
 *
 * Example:
 * @code
 * a='x\'
 * @endcode
 *
 * Produces kUnterminatedQuotedValue. The backslash escapes the quote for quote
 * matching, so the parser reaches the end of input without a closing quote.
 *
 * Important notes:
 * - `a=` is valid and produces an empty value;
 * - `a=''` and `a=""` are also valid and produce an empty value;
 * - `a= b=2` is parsed as two items: `a` with an empty value and `b=2`;
 * - `a = 1` is parsed as one item: `a=1`;
 * - `a='x'b=1` is valid and is parsed as two items: `a=x` and `b=1`;
 * - quoted keys are supported, but their escape sequences are not decoded;
 * - empty quoted key, such as `''=x`, is reported as kEmptyKey;
 * - for quoted keys and quoted values, ParamTokenView::begin/end point to the
 *   content inside the outer quotes, not to the quote characters themselves;
 * - unterminated quoted key is reported as kUnterminatedQuotedKey and stops
 *   parsing, because there is no deterministic way to know where the broken
 *   quoted key should end;
 * - unterminated quoted value is reported as kUnterminatedQuotedValue and stops
 *   parsing, because there is no deterministic way to know where the broken
 *   quoted value should end;
 * - if copy_input=false, returned views reference the input buffer.
 */
ParamParseResult ParseParamString(const std::string_view _text, const ParamParseOptions _options = {});

/**
 * @brief Parses a null-terminated C string into items and flags.
 *
 * @param _text Input C string. May be nullptr.
 * @param _options Parser options.
 * @return Soft parse result containing parsed tokens and collected errors.
 *
 * @details
 * If `_text == nullptr`, the function returns an empty result with one
 * kNullInput error at position 0.
 */
ParamParseResult ParseParamString(const char* _text, const ParamParseOptions _options = {});

} // namespace xsdk::xbase
