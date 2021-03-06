exception UnexpectedBlockType(string);
exception UnexpectedRequestSource(string);

type markdown = string;
type url = string;
type title = string;
type caption = string;
type embedCode = option(string);
type filename = string;
type requestSource = [ | `User | `VimeoUpload];
type lastResolvedAt = option(Js.Date.t);

type blockType =
  | Markdown(markdown)
  | File(url, title, filename)
  | Image(url, caption)
  | Embed(url, embedCode, requestSource, lastResolvedAt);

type t = {
  id,
  blockType,
  sortIndex: int,
}
and id = string;

let decodeMarkdownContent = json =>
  Json.Decode.(json |> field("markdown", string));
let decodeFileContent = json => Json.Decode.(json |> field("title", string));

let decodeImageContent = json =>
  Json.Decode.(json |> field("caption", string));

let decodeEmbedContent = json => {
  let requestSourceString =
    Json.Decode.(field("requestSource", string, json));

  let requestSource =
    switch (requestSourceString) {
    | "User" => `User
    | "VimeoUpload" => `VimeoUpload
    | otherRequestSource =>
      Rollbar.error(
        "Unexpected requestSource encountered in ContentBlock.re: "
        ++ otherRequestSource,
      );
      raise(UnexpectedRequestSource(otherRequestSource));
    };

  Json.Decode.(
    json |> field("url", string),
    json |> optional(field("embedCode", string)),
    requestSource,
    json |> optional(field("lastResolvedAt", DateFns.decodeISO)),
  );
};

let decode = json => {
  open Json.Decode;

  let blockType =
    switch (json |> field("blockType", string)) {
    | "markdown" => Markdown(json |> field("content", decodeMarkdownContent))
    | "file" =>
      let title = json |> field("content", decodeFileContent);
      let url = json |> field("fileUrl", string);
      let filename = json |> field("filename", string);
      File(url, title, filename);
    | "image" =>
      let caption = json |> field("content", decodeImageContent);
      let url = json |> field("fileUrl", string);
      Image(url, caption);
    | "embed" =>
      let (url, embedCode, requestSource, lastResolvedAt) =
        json |> field("content", decodeEmbedContent);
      Embed(url, embedCode, requestSource, lastResolvedAt);
    | unknownBlockType => raise(UnexpectedBlockType(unknownBlockType))
    };

  {
    id: json |> field("id", string),
    blockType,
    sortIndex: json |> field("sortIndex", int),
  };
};

let sort = blocks =>
  blocks |> ArrayUtils.copyAndSort((x, y) => x.sortIndex - y.sortIndex);

let id = t => t.id;
let blockType = t => t.blockType;
let sortIndex = t => t.sortIndex;

let makeMarkdownBlock = markdown => Markdown(markdown);
let makeImageBlock = (fileUrl, caption) => Image(fileUrl, caption);
let makeFileBlock = (fileUrl, title, fileName) =>
  File(fileUrl, title, fileName);
let makeEmbedBlock = (url, embedCode, requestSource, lastResolvedAt) =>
  Embed(url, embedCode, requestSource, lastResolvedAt);

let make = (id, blockType, sortIndex) => {id, blockType, sortIndex};

let makeFromJs = js => {
  let id = js##id;
  let sortIndex = js##sortIndex;
  let blockType =
    switch (js##content) {
    | `MarkdownBlock(content) => Markdown(content##markdown)
    | `FileBlock(content) =>
      File(content##url, content##title, content##filename)
    | `ImageBlock(content) => Image(content##url, content##caption)
    | `EmbedBlock(content) =>
      Embed(
        content##url,
        content##embedCode,
        content##requestSource,
        content##lastResolvedAt->Belt.Option.map(DateFns.parseISO),
      )
    };

  make(id, blockType, sortIndex);
};

let blockTypeAsString = blockType =>
  switch (blockType) {
  | Markdown(_markdown) => "markdown"
  | File(_url, _title, _filename) => "file"
  | Image(_url, _caption) => "image"
  | Embed(_url, _embedCode, _requestSource, _lastResolvedAt) => "embed"
  };

let incrementSortIndex = t => {...t, sortIndex: t.sortIndex + 1};

let reindex = ts => ts |> List.mapi((sortIndex, t) => {...t, sortIndex});

let moveUp = (t, ts) =>
  ts
  |> sort
  |> Array.to_list
  |> ListUtils.swapUp(t)
  |> reindex
  |> Array.of_list;

let moveDown = (t, ts) =>
  ts
  |> sort
  |> Array.to_list
  |> ListUtils.swapDown(t)
  |> reindex
  |> Array.of_list;

let updateFile = (title, t) =>
  switch (t.blockType) {
  | File(url, _, filename) => {...t, blockType: File(url, title, filename)}
  | Markdown(_)
  | Image(_)
  | Embed(_) => t
  };

let updateImage = (caption, t) =>
  switch (t.blockType) {
  | Image(url, _) => {...t, blockType: Image(url, caption)}
  | Markdown(_)
  | File(_)
  | Embed(_) => t
  };

let updateMarkdown = (markdown, t) =>
  switch (t.blockType) {
  | Markdown(_) => {...t, blockType: Markdown(markdown)}
  | File(_)
  | Image(_)
  | Embed(_) => t
  };

module Fragments = [%graphql
  {|
  fragment allFields on ContentBlock {
    id
    blockType
    sortIndex
    content {
      ... on ImageBlock {
        caption
        url
        filename
      }
      ... on FileBlock {
        title
        url
        filename
      }
      ... on MarkdownBlock {
        markdown
      }
      ... on EmbedBlock {
        url
        embedCode
        requestSource
        lastResolvedAt
      }
    }
  }
|}
];

module Query = [%graphql
  {|
    query ContentBlocksWithVersionsQuery($targetId: ID!, $targetVersionId: ID) {
      contentBlocks(targetId: $targetId, targetVersionId: $targetVersionId) {
        id
        blockType
        sortIndex
        content {
          ... on ImageBlock {
            caption
            url
            filename
          }
          ... on FileBlock {
            title
            url
            filename
          }
          ... on MarkdownBlock {
            markdown
          }
          ... on EmbedBlock {
            url
            embedCode
            requestSource
            lastResolvedAt
          }
        }
      }
      targetVersions(targetId: $targetId){
        id
        createdAt
        updatedAt
      }
  }
|}
];
